#include "gpu.h"

#include "imgui.h"

#include "common/log.h"
#include "common/asserts.h"
#include "renderer/renderer_sw.h"
#include "interrupt.h"
#include "system.h"
#include "timer/timers.h"

LOG_CHANNEL(GPU);

GPU::GPU(System* system) : sys(system), vram(VRAM_SIZE, 0), output(VRAM_SIZE * 2, 0) {
    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;

    // load default renderer backend
    renderer = std::make_unique<Renderer_SW>(this);

    // register timed event
    sys->RegisterEvent(
        System::TimedEvent::GPU, [this](u32 cycles) { Step(cycles); }, [this]() { return CyclesUntilNextEvent(); });
}

void GPU::Step(u32 cpu_cycles) {
    DebugAssert(cpu_cycles <= cycles_until_next_event);

    auto [gpu_cycles, dots] = GpuCyclesAndDots(cpu_cycles);

    accumulated_dots += dots;

    const float dots_per_scanline = DotsPerScanline();
    while (accumulated_dots >= dots_per_scanline) {
        accumulated_dots -= dots_per_scanline;
        scanline = (scanline + 1) % Scanlines();

        // if vertical interlace is on AND the vertical resolution is set to 240
        // the even_odd_bit needs to be flipped every scanline
        if (InterlacedAnd240Vres()) status.interlace_even_or_odd_line ^= 1;
    }

    auto& dot_timer = sys->timers->dot_timer;
    if (!dot_timer.IsUsingSystemClock()) {
        static float dotclock_dots = 0;
        dotclock_dots += dots;
        dot_timer.Increment(static_cast<u32>(dotclock_dots));
        dotclock_dots = std::fmod(dotclock_dots, 1.0f);
    }

    const bool currently_in_hblank = IsGpuInHblank();

    if (dot_timer.mode.sync_enabled) {
        dot_timer.UpdateBlankState(currently_in_hblank);
    }

    auto& hblank_timer = sys->timers->hblank_timer;
    if (!hblank_timer.IsUsingSystemClock()) {
        const u32 lines = static_cast<u32>(dots / dots_per_scanline);
        hblank_timer.Increment(static_cast<u32>(currently_in_hblank && !was_in_vblank) + lines);
    }

    was_in_hblank = currently_in_hblank;

    const bool currently_in_vblank = IsGpuInVblank();

    if (hblank_timer.mode.sync_enabled) {
        hblank_timer.UpdateBlankState(currently_in_vblank);
    }

    if (!was_in_vblank && currently_in_vblank) {
        sys->interrupt->Request(IRQ::VBLANK);

        draw_frame = true;

        // flip the interlace bit once every frame if vres is 480
        // we do not need to flip the bit again here in 240 vres 'mode'
        // because it reaches the same state after flipping 240 times
        if (!InterlacedAnd240Vres()) status.interlace_even_or_odd_line ^= 1;
    }

    was_in_vblank = currently_in_vblank;
}

u32 GPU::CyclesUntilNextEvent() {
    float gpu_cycles_until_next_event = std::numeric_limits<float>::max();

    auto MinCycles = [&](float new_cycles) {
        gpu_cycles_until_next_event = std::min(gpu_cycles_until_next_event, new_cycles);
    };

    const float dots_per_cycle = DotsPerGpuCycle();
    const float dots_per_scanline = DotsPerScanline();

    const float horizontal_res = static_cast<float>(HorizontalRes());

    auto& dot_timer = sys->timers->dot_timer;
    if (dot_timer.mode.sync_enabled) {
        // synced with hblank
        const float cycles_until_hblank_flip =
            ((accumulated_dots < horizontal_res ? horizontal_res : dots_per_scanline) - accumulated_dots) /
            dots_per_cycle;
        MinCycles(cycles_until_hblank_flip);
    }

    if (!dot_timer.paused && !dot_timer.IsUsingSystemClock()) {
        const float cycles_until_irq = dot_timer.CyclesUntilNextIRQ() / dots_per_cycle;
        MinCycles(cycles_until_irq);
    }

    const u32 lines_until_vblank_flip = (scanline < 240 ? 240 : Scanlines()) - scanline;
    const float cycles_until_vblank_flip =
        lines_until_vblank_flip * GpuCyclesPerScanline() - accumulated_dots / dots_per_cycle;
    MinCycles(cycles_until_vblank_flip);

    auto& hblank_timer = sys->timers->hblank_timer;
    if (!hblank_timer.paused && !hblank_timer.IsUsingSystemClock()) {
        const float cycles_until_hblank =
            ((accumulated_dots < horizontal_res ? horizontal_res : dots_per_scanline + horizontal_res) -
             accumulated_dots) /
            dots_per_cycle;
        const float cycles_until_irq = hblank_timer.CyclesUntilNextIRQ() * GpuCyclesPerScanline() - cycles_until_hblank;
        MinCycles(cycles_until_irq);
    }

    cycles_until_next_event = static_cast<u32>(std::ceil(gpu_cycles_until_next_event / (11.0 / 7.0)));

    return cycles_until_next_event;
}

void GPU::SendGP0Cmd(u32 cmd) {
    // move all this logic into dma.cpp
    if (mode == Mode::DataToCPU || mode == Mode::DataFromCPU) {
        if (words_remaining == 0) {
            mode = Mode::Command;
            command_counter = 0;
        } else {
            if (mode == Mode::DataFromCPU) CopyRectCpuToVram(cmd);
            if (mode == Mode::DataToCPU) CopyRectVramToCpu();
            words_remaining--;
            // no reason to continue
            return;
        }
    }

    //LOG_DEBUG << fmt::format("GPU received GP0 command 0x{:08X}", cmd);
    if (command_counter == 0) {
        command.value = cmd;
    }
    DebugAssert(command_counter < 12);
    command_buffer[command_counter] = cmd;

    auto CommandAfterCount = [this](u32 count, auto function) {
        if (command_counter == count) {
            function();
            command_counter = 0;
        } else {
            command_counter++;
        }
    };

    // only used for poly-line draw commands
    auto CommandAfterTerminationCode = [this](auto function) {
        if ((command_buffer[command_counter] & TERM_CODE_MASK) == TERM_CODE) {
            function();
            command_counter = 0;
        } else {
            command_counter++;
        }
    };

    // clang-format off

    switch (command.gp0_op) {
        case 0x00: // nop
            break;
        case 0x01: // clear cache
            // unimplemented, clears the 2KiB texture cache which is also not implemented
            break;
        case 0x02: // fill vram
            CommandAfterCount(2, [this]() { FillVram(); });
            break;
        case 0x1F: // interrupt request
            LogWarn("Interrupt request [Unimplemented]");
            break;
        case 0x20: case 0x22: // mono triangle
            CommandAfterCount(3, [this]() { DrawPolygonMono<Three_Point>(); });
            break;
        case 0x28: case 0x2A: // mono quad
            CommandAfterCount(4, [this]() { DrawPolygonMono<Four_Point>(); });
            break;
        case 0x24: case 0x25: case 0x26: case 0x27:  // textured triangle
            CommandAfterCount(6, [this]() { DrawPolygonTextured<Three_Point>(); });
            break;
        case 0x2C: case 0x2D: case 0x2E: case 0x2F: // textured quad
            CommandAfterCount(8, [this]() { DrawPolygonTextured<Four_Point>(); });
            break;
        case 0x30: case 0x32: // shaded triangle
            CommandAfterCount(5, [this]() { DrawPolygonShaded<Three_Point>(); });
            break;
        case 0x38: case 0x3A: // shaded quad
            CommandAfterCount(7, [this]() { DrawPolygonShaded<Four_Point>(); });
            break;
        case 0x34: case 0x36: // textured and shaded triangle
            CommandAfterCount(8, [this]() { DrawPolygonTexturedShaded<Three_Point>(); });
            break;
        case 0x3C: case 0x3E: // textured and shaded quad
            CommandAfterCount(11, [this]() { DrawPolygonTexturedShaded<Four_Point>(); });
            break;
        case 0x40: case 0x42: // mono single line
            CommandAfterCount(2, [this]() { DrawLineMono(false); });
            break;
        case 0x48: case 0x4A: // mono poly line
            CommandAfterTerminationCode([this]() { DrawLineMono(true); });
            break;
        case 0x50: case 0x52: // shaded single line
            CommandAfterCount(3, [this]() { DrawLineShaded(false); });
            break;
        case 0x58: case 0x5A: // shaded poly line
            CommandAfterTerminationCode([this]() { DrawLineShaded(true); });
            break;
        case 0x60: case 0x62: // mono rectangle with variable size
            CommandAfterCount(2, [this]() { DrawRectangleMono(); });
            break;
        case 0x68: case 0x6A: case 0x70: case 0x72: // mono rectangle with fixed size
        case 0x78: case 0x7A:
            CommandAfterCount(1, [this]() { DrawRectangleMono(); });
            break;
        case 0x64: case 0x65: case 0x66: case 0x67: // textured rectangle with variable size
            CommandAfterCount(3, [this]() { DrawRectangleTextured(); });
            break;
        case 0x74: case 0x75: case 0x76: case 0x77: // textured rectangle with fixed size
        case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            CommandAfterCount(2, [this]() { DrawRectangleTextured(); });
            break;
        case 0x80: // copy rectangle from vram to vram
            CommandAfterCount(3, [this]() { CopyRectVramToVram(); });
            break;
        case 0xA0: // copy rectangle from ram to vram
            CommandAfterCount(2, [this]() { CopyRectCpuToVram(); });
            break;
        case 0xC0: // copy rectangle from vram to ram
            CommandAfterCount(2, [this]() { CopyRectVramToCpu(); });
            break;
        case 0xE1: // draw mode setting
            // TODO: use status.value for this and similar commands
            status.tex_page_x_base = cmd & 0xF;
            status.tex_page_y_base = (cmd >> 4) & 0x1;
            status.semi_transparency = (cmd >> 5) & 0x3;
            status.tex_page_colors = (cmd >> 7) & 0x3;
            status.dither = (cmd >> 9) & 0x1;
            status.draw_enable = (cmd >> 10) & 0x1;
            status.tex_disable = (cmd >> 11) & 0x1;
            tex_rectangle_xflip = (cmd >> 12) & 0x1;
            tex_rectangle_yflip = (cmd >> 13) & 0x1;
            break;
        case 0xE2:    // texture window setting
            tex_window_x_mask = cmd & 0x1F;
            tex_window_y_mask = (cmd >> 5) & 0x1F;
            tex_window_x_offset = (cmd >> 10) & 0x1F;
            tex_window_y_offset = (cmd >> 15) & 0x1F;
            break;
        case 0xE3:    // drawing area top left
            drawing_area_top = static_cast<u16>((cmd >> 10) & 0x1FF);
            drawing_area_left = static_cast<u16>(cmd & 0x3FF);
            //LOG_DEBUG << "Set draw area top=" << drawing_area_top << ", left=" << drawing_area_left;
            break;
        case 0xE4:    // drawing area bottom right
            drawing_area_bottom = static_cast<u16>((cmd >> 10) & 0x1FF);
            drawing_area_right = static_cast<u16>(cmd & 0x3FF);
            //LOG_DEBUG << "Set draw area bottom=" << drawing_area_bottom << ", right=" << drawing_area_right;
            break;
        case 0xE5:    // drawing offset
        {
            const u16 x = static_cast<u16>(cmd & 0x7FF);
            const u16 y = static_cast<u16>((cmd >> 11) & 0x7FF);
            drawing_x_offset = static_cast<s16>(x << 5) >> 5;
            drawing_y_offset = static_cast<s16>(y << 5) >> 5;
            break;
        }
        case 0xE6:    // mask bit setting
            status.mask_enable = cmd & 0x1;
            status.draw_pixels = (cmd >> 1) & 0x1;
            break;
        default:
            LogWarn("Invalid GP0 command 0x{:08X}", cmd);
    }

    // clang-format on
}

void GPU::SendGP1Cmd(u32 cmd) {
    //LOG_DEBUG << fmt::format("GPU received GP1 command 0x{:08X}", cmd);
    command.value = cmd;

    // clang-format off

    switch (command.gp1_op) {
        case Gp1Command::reset:
            ResetCommand();
            break;
        case Gp1Command::cmd_buf_reset:
            LogDebug("Reset command buffer [Unimplemented]");
            break;
        case Gp1Command::ack_gpu_interrupt:
            LogDebug("ACK GPU Interrupt [Unimplemented]");
            break;
        case Gp1Command::display_enable:
            status.display_disabled = cmd & 0x1;
            break;
        case Gp1Command::dma_dir:
            status.dma_direction = static_cast<DmaDirection>(cmd & 0x3);
            break;
        case Gp1Command::display_vram_start:
            display_vram_x_start = cmd & 0x3FE;
            display_vram_y_start = (cmd >> 10) & 0x1FF;
            break;
        case Gp1Command::display_hor_range:
            display_horizontal_start = cmd & 0xFFF;
            display_horizontal_end = (cmd >> 12) & 0xFFF;
            break;
        case Gp1Command::display_ver_range:
            display_line_start = cmd & 0x3FF;
            display_line_end = (cmd >> 10) & 0x3FF;
            break;
        case Gp1Command::display_mode:
        {
            GpuStatus new_status;
            new_status.value = status.value;

            new_status.horizontal_res_1 = cmd & 0x3;
            new_status.vertical_res = (cmd >> 2) & 0x1;
            new_status.video_mode = static_cast<VideoMode>((cmd >> 3) & 0x1);
            new_status.display_area_color_depth = (cmd >> 4) & 0x1;
            new_status.vertical_interlace = (cmd >> 5) & 0x1;
            new_status.horizontal_res_2 = (cmd >> 6) & 0x1;
            if ((cmd >> 7) & 0x1) {
                LogWarn("Unimplemented: Toggled GPUSTAT.14 (Reverse Flag)");
            }

            if (new_status.value != status.value) {
                sys->ForceUpdateComponents();
                status.value = new_status.value;
                sys->RecalculateCyclesUntilNextEvent();
            }

            break;
        }
        case Gp1Command::gpu_info:
            // we pretend to have the older 160pin GPU
            // TODO: differences between old and new GPU?
            // TODO: remaining options
            switch (cmd & 0xFFFFFF) {
                case 7: break;
                default: Panic("Unimplemented GPU info option {}", cmd & 0xFFFFFF);
            }
            break;
        default:
            Panic("Unimplemented GP1 command 0x{:08X}", cmd);
    }

    // clang-format on
}

//              //
//  POLYGONS    //
//              //

template<GPU::PolygonType type>
void GPU::DrawPolygonMono() {
    static_assert(type == Three_Point || type == Four_Point);

    //LOG_DEBUG << "DrawPolygonMono";

    for (u8 i = 0; i < static_cast<u8>(type); i++) {
        vertices[i].SetPoint(command_buffer[i + 1]);
        vertices[i].c.SetColor(command_buffer[0]);
        vertices[i].SetTextPoint(0);
    }
    renderer->Draw(command_buffer[0]);
}

template<GPU::PolygonType type>
void GPU::DrawPolygonShaded() {
    static_assert(type == Three_Point || type == Four_Point);

    //LOG_DEBUG << "DrawPolygonShaded";

    for (u8 i = 0; i < static_cast<u8>(type); i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[(i * 2)]);
        vertices[i].SetTextPoint(0);
    }
    renderer->Draw(command_buffer[0]);
}

template<GPU::PolygonType type>
void GPU::DrawPolygonTextured() {
    static_assert(type == Three_Point || type == Four_Point);

    //LOG_DEBUG << "DrawPolygonTextured";

    clut = command_buffer[2] >> 16;

    // TODO: does this actually modify the status register?
    u16 texpage_attribute = command_buffer[4] >> 16;
    status.tex_page_x_base = texpage_attribute & 0xF;
    status.tex_page_y_base = (texpage_attribute >> 4) & 0x1;
    status.semi_transparency = (texpage_attribute >> 5) & 0x3;
    status.tex_page_colors = (texpage_attribute >> 7) & 0x3;
    status.tex_disable = (texpage_attribute >> 11) & 0x1;

    for (u8 i = 0; i < static_cast<u8>(type); i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[0]);
        vertices[i].SetTextPoint(command_buffer[(i * 2) + 2]);
    }

    renderer->Draw(command_buffer[0]);
}

template<GPU::PolygonType type>
void GPU::DrawPolygonTexturedShaded() {
    static_assert(type == Three_Point || type == Four_Point);

    //LOG_DEBUG << "DrawPolygonTexturedShaded";

    clut = command_buffer[2] >> 16;

    // TODO: does this actually modify the status register?
    u16 texpage_attribute = command_buffer[5] >> 16;
    status.tex_page_x_base = texpage_attribute & 0xF;
    status.tex_page_y_base = (texpage_attribute >> 4) & 0x1;
    status.semi_transparency = (texpage_attribute >> 5) & 0x3;
    status.tex_page_colors = (texpage_attribute >> 7) & 0x3;
    status.tex_disable = (texpage_attribute >> 11) & 0x1;

    for (u8 i = 0; i < static_cast<u8>(type); i++) {
        vertices[i].SetPoint(command_buffer[(i * 3) + 1]);
        vertices[i].c.SetColor(command_buffer[i * 3]);
        vertices[i].SetTextPoint(command_buffer[(i * 3) + 2]);
    }

    renderer->Draw(command_buffer[0]);
}

//              //
//  RECTANGLES  //
//              //

void GPU::DrawRectangleMono() {
    //LOG_DEBUG << "DrawRectangleMono";

    rectangle.c.SetColor(command_buffer[0]);
    rectangle.SetStart(command_buffer[1]);
    rectangle.SetSize(command_buffer[2]);

    renderer->Draw(command_buffer[0]);
}

void GPU::DrawRectangleTextured() {
    //LOG_DEBUG << "DrawRectangleTexture";

    rectangle.c.SetColor(command_buffer[0]);
    rectangle.SetStart(command_buffer[1]);
    rectangle.SetTextPoint(command_buffer[2]);
    rectangle.SetSize(command_buffer[3]);

    // Texpage gets set up separately via GP0(0xE1)
    clut = command_buffer[2] >> 16;

    renderer->Draw(command_buffer[0]);
}

//              //
//  LINES       //
//              //

void GPU::DrawLineMono(bool is_poly_line) {
    line_buffer.clear();
    //LogDebug("DrawLineMono (is_poly_line={})", is_poly_line);

    if (is_poly_line) DebugAssert((command_buffer[command_counter] & TERM_CODE_MASK) == TERM_CODE);

    u32 num_points = is_poly_line ? (command_counter - 1) : 2;

    for (u32 i = 0; i < num_points; i++) {
        Vertex p = {};
        p.SetPoint(command_buffer[i+1]);
        p.c.SetColor(command_buffer[0]);

        line_buffer.emplace_back(p);
    }

    renderer->Draw(command_buffer[0]);
}

void GPU::DrawLineShaded(bool is_poly_line) {
    line_buffer.clear();
    //LogDebug("DrawLineShaded (is_poly_line={})", is_poly_line);

    if (is_poly_line) DebugAssert((command_buffer[command_counter] & TERM_CODE_MASK) == TERM_CODE);

    u32 num_points = is_poly_line ? (command_counter / 2) : 2;

    for (u32 i = 0; i < num_points; i++) {
        Vertex p = {};
        p.SetPoint(command_buffer[i * 2 + 1]);
        p.c.SetColor(command_buffer[i * 2]);

        line_buffer.emplace_back(p);
    }

    renderer->Draw(command_buffer[0]);
}

void GPU::CopyRectCpuToVram(u32 data /* = 0 */) {
    static u32 x_pos_max = 0;
    static u32 x_pos = 0;
    static u32 y_pos = 0;

    if (mode == Mode::Command) {
        const u32 pos = command_buffer[1];
        x_pos = pos & 0x3FF;
        y_pos = (pos >> 16) & 0x1FF;

        const u32 resolution = command_buffer[2];
        const u32 width = resolution & 0x3FF;
        const u32 height = (resolution >> 16) & 0x1FF;
        u32 img_size = width * height;
        // round up
        img_size = (img_size + 1) & ~0x1;
        // the GPU uses 16-bit pixels but receives them in 32-bit packets
        words_remaining = img_size / 2;

        // determine the length of a line (can overflow VRAM_WIDTH in which case x_pos must wrap around)
        x_pos_max = x_pos + width;

        mode = Mode::DataFromCPU;
        //LogDebug("CopyCPUtoVram: {} words from (x={}, y={}) to (x={}, y={})",
        //                         words_remaining, x_pos, y_pos, x_pos + width - 1, y_pos + height - 1);
    } else if (mode == Mode::DataFromCPU) {
        // first halfword
        vram[(x_pos % VRAM_WIDTH) + VRAM_WIDTH * y_pos] = data & 0xFFFF;
        // increment and check for overflow
        auto Increment = [&] {
            x_pos++;
            if (x_pos >= x_pos_max) {
                y_pos = (y_pos + 1) % 512;
                x_pos = command_buffer[1] & 0x3FF;
            }
        };
        Increment();

        // second halfword
        vram[(x_pos % VRAM_WIDTH) + VRAM_WIDTH * y_pos] = data >> 16;
        Increment();
    } else {
        Panic("Invalid GPU transfer mode during CopyRectCpuToVram");
    }
}

void GPU::CopyRectVramToCpu() {
    static u32 x_pos_max = 0;
    static u32 x_pos = 0;
    static u32 y_pos = 0;

    if (mode == Mode::Command) {
        const u32 pos = command_buffer[1];
        x_pos = pos & 0x3FF;
        y_pos = (pos >> 16) & 0x1FF;

        const u32 resolution = command_buffer[2];
        const u32 width = resolution & 0x3FF;
        const u32 height = (resolution >> 16) & 0x1FF;
        u32 img_size = width * height;
        // round up
        img_size = (img_size + 1) & ~0x1;
        // the GPU uses 16-bit pixels but sends them in 32-bit packets
        words_remaining = img_size / 2;

        // determine the length of a line (can overflow VRAM_WIDTH in which case x_pos must wrap around)
        x_pos_max = x_pos + width;

        mode = Mode::DataToCPU;
        //LogDebug("CopyVramToCPU: {} words from (x={}, y={}) to (x={}, y={})", words_remaining, x_pos,
        //                         y_pos, x_pos + width - 1, y_pos + height - 1);
    } else if (mode == Mode::DataToCPU) {
        // first halfword
        u32 word1 = vram[(x_pos % VRAM_WIDTH) + VRAM_WIDTH * y_pos];
        // increment and check for overflow
        auto Increment = [&] {
            x_pos++;
            if (x_pos >= x_pos_max) {
                y_pos = (y_pos + 1) % 512;
                x_pos = command_buffer[1] & 0x3FF;
            }
        };
        Increment();

        // second halfword
        u32 word2 = vram[(x_pos % VRAM_WIDTH) + VRAM_WIDTH * y_pos];
        Increment();

        gpu_read = (word2 << 16) | word1;
    } else {
        Panic("Invalid GPU transfer mode during CopyRectVramToCpu");
    }
}

void GPU::CopyRectVramToVram() {
    const u32 src_coords = command_buffer[1];
    const u32 src_start_x = src_coords & 0x3FF;
    const u32 src_start_y = (src_coords >> 16) & 0x1FF;

    const u32 dst_coords = command_buffer[2];
    const u32 dst_start_x = dst_coords & 0x3FF;
    const u32 dst_start_y = (dst_coords >> 16) & 0x1FF;

    const u32 size = command_buffer[3];
    u32 size_x = size & 0x3FF;
    size_x = size_x == 0 ? 0x400 : ((size_x - 1) & 0x3FF) + 1;
    u32 size_y = (size >> 16) & 0x1FF;
    size_y = size_y == 0 ? 0x200 : ((size_y - 1) & 0x1FF) + 1;

    for (u32 src_y = src_start_y, dst_y = dst_start_y; src_y < src_start_y + size_y; src_y++, dst_y++) {
        src_y &= 0x1FF;
        dst_y &= 0x1FF;
        for (u32 src_x = src_start_x, dst_x = dst_start_x; src_x < src_start_x + size_x; src_x++, dst_x++) {
            src_x &= 0x3FF;
            dst_x &= 0x3FF;
            vram[dst_y * VRAM_WIDTH + dst_x] = vram[src_y * VRAM_WIDTH + src_x];
        }
    }
}

void GPU::FillVram() {
    const u32 coords = command_buffer[1];
    const u32 start_x = coords & 0x3F0;                     // range 0..0x3F0 (fill x works in steps of 0x10)
    const u32 start_y = (coords >> 16) & 0x1FF;             // range 0..0x1FF

    const u32 size = command_buffer[2];
    const u32 size_x = ((size & 0x3FF) + 0xF) & ~0xF;     // range 0..0x400 (in steps of 0x10)
    const u32 size_y = (size >> 16) & 0x1FF;              // range 0..0x1FF

    if (size_x == 0 || size_y == 0) return;

    Color c = {};
    c.SetColor(command_buffer[0]);
    const u16 color_15bit = c.To5551();

    //LogDebug("FillRectVram: start_x={}, start_y={}, size_x={}, size_y={}, color=0x{:08x}", start_x, start_y, size_x, size_y,
    //         command_buffer[0] & 0x00FFFFFF);

    for (u32 y = start_y; y < start_y + size_y; y++) {
        y &= 0x1FF;
        for (u32 x = start_x; x < start_x + size_x; x++) {
            x &= 0x3FF;
            vram[y * VRAM_WIDTH + x] = color_15bit;
        }
    }
}

void GPU::ResetCommand() {
    // TODO: FIFO and texture cache
    // TODO: just set status.value to zero?
    status.interrupt_request = false;
    status.tex_page_x_base = 0;
    status.tex_page_y_base = 0;
    status.semi_transparency = 0;
    status.tex_page_colors = 0;

    tex_window_x_mask = 0;
    tex_window_y_mask = 0;
    tex_window_x_offset = 0;
    tex_window_y_offset = 0;

    status.dither = false;
    status.draw_enable = false;
    status.tex_disable = false;
    tex_rectangle_xflip = false;
    tex_rectangle_yflip = false;

    drawing_area_left = 0;
    drawing_area_top = 0;
    drawing_area_right = 0;
    drawing_area_bottom = 0;
    drawing_x_offset = 0;
    drawing_y_offset = 0;

    status.mask_enable = false;
    status.draw_pixels = false;
    status.dma_direction = DmaDirection::Off;
    status.display_disabled = true;
    display_vram_x_start = 0;
    display_vram_y_start = 0;
    status.horizontal_res_1 = 0;
    status.horizontal_res_2 = 0;
    status.vertical_res = 0;
    status.video_mode = VideoMode::NTSC;
    status.vertical_interlace = true;

    display_horizontal_start = 0x200;
    display_horizontal_end = 0xC00;
    display_line_start = 0x10;
    display_line_end = 0x100;
    status.display_area_color_depth = 0;
}

u32 GPU::ReadStat() {
    // the even_odd_bit might flip again between now and the next timed event (e.g. a timer reaching its target)
    // TODO: make this a config? performance vs accuracy...
    const auto dots_until_next_event = accumulated_dots + sys->GetCyclesUntilNextEvent() * DotsPerGpuCycle();
    if (dots_until_next_event >= DotsPerScanline()) {
        sys->ForceUpdateComponents();
        sys->RecalculateCyclesUntilNextEvent();
    }

    // even_odd_bit is always 0 during vblank
    return status.value & ~(static_cast<u32>(was_in_vblank) << 31);
}

u16* GPU::GetVRAM() {
    return vram.data();
}

u8* GPU::GetVideoOutput() {
    const u32 hres = HorizontalRes();
    const u32 vres = VerticalRes();
    const usize size = HorizontalRes() * VerticalRes();

    if (output.size() != size) {
        LogDebug("Changing display size to {}x{}", hres, vres);
        output.resize(size);
        //output.clear();
    }

    const u32 start_y = display_vram_y_start;
    const u32 end_y = start_y + vres;
    DebugAssert(end_y < 512);

    // VRAM data is indexed as halfwords (u16)
    const u32 start_x = display_vram_x_start;
    const u32 row_bytes = status.display_area_color_depth ? (hres * 3) : (hres * 2);
    DebugAssert(start_x * 2 + row_bytes < 2048);

    // copy display area from vram to output
    for (u32 y = start_y, dest_i = 0; y < end_y; y++, dest_i++) {
        std::memcpy(output.data() + row_bytes * dest_i, (u8*)(vram.data() + VRAM_WIDTH * y + start_x), row_bytes);
    }

    return output.data();
}

void GPU::Reset() {
    draw_frame = false;

    gpu_read = 0;
    status.value = 0;

    drawing_area_left = 0;
    drawing_area_top = 0;
    drawing_area_right = 0;
    drawing_area_bottom = 0;
    drawing_x_offset = 0;
    drawing_y_offset = 0;
    // TODO: reset variables once they will actually be used

    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;

    command.value = 0;
    command_counter = 0;
    for (auto& c : command_buffer) c = 0;
    mode = Mode::Command;
    words_remaining = 0;

    gpu_clock = 0;
    scanline = 0;

    cycles_until_next_event = 0;
    accumulated_dots = 0.0f;

    was_in_hblank = was_in_vblank = false;

    std::fill(vram.begin(), vram.end(), 0);
    std::fill(output.begin(), output.end(), 0);

    for (auto& v : vertices) v.Reset();

    line_buffer.clear();

    clut = 0;
}

void GPU::DrawGpuState(bool* open) {
    ImGui::Begin("GPU State", open);

    ImGui::Columns(2);
    ImGui::Text("Command buffer");
    ImGui::Separator();
    const ImVec4 white(1.0, 1.0, 1.0, 1.0);
    const ImVec4 grey(0.5, 0.5, 0.5, 1.0);
    for (u32 i = 0; i < 12; i++) {
        ImGui::TextColored((i > command_counter) ? grey : white, " %-2d  %08X", i, command_buffer[i]);
        if (i == command_counter) {
            ImGui::SameLine();
            ImGui::Text("  <--- cmd end");
        }
    }
    ImGui::NextColumn();
    if (ImGui::TreeNode("__gpustat_node", "GPUSTAT   0x%08X", status.value)) {
        ImGui::Text("Tex page base: x=%u, y=%u", status.tex_page_x_base * 64, status.tex_page_y_base * 256);
        ImGui::Text("Semi transparency: %u", status.semi_transparency.GetValue());
        static const char* const formats[4] = {"4bit", "8bit", "15bit", "Invalid"};
        ImGui::Text("Tex page format: %s", formats[status.tex_page_colors]);
        ImGui::Text("Dithering: %s", status.dither ? "Enabled" : "Off");
        ImGui::Text("Draw to display area: %s", status.draw_enable ? "Allowed" : "Prohibited");
        ImGui::Text("Mask bit enabled: %s", status.mask_enable ? "Yes" : "No");
        ImGui::Text("Draw to masked areas: %s", status.draw_pixels ? "No" : "Yes");
        ImGui::Text("Interlace field: %u", status.interlace_field.GetValue());
        ImGui::Text("Reverse flag: %u", status.reverse.GetValue());
        ImGui::Text("Textures enabled: %s", status.tex_disable ? "No" : "Yes");
        ImGui::Text("Vertical interlace: %u", status.vertical_interlace.GetValue());
        ImGui::Text("Display enabled: %s", status.display_disabled ? "No" : "Yes");
        ImGui::Text("Interrupt Request: %s", status.interrupt_request ? "IRQ1" : "Off");
        ImGui::Text("DMA / Data Request: %u", status.dma_data_stat.GetValue());
        ImGui::Text("Ready to receive command: %u", status.can_receive_cmd_word.GetValue());
        ImGui::Text("Ready to send VRAM: %u", status.can_send_vram_to_cpu.GetValue());
        ImGui::Text("Ready to receive DMA block: %u", status.can_receive_dma_block.GetValue());
        ImGui::Text("DMA direction: %u", static_cast<u32>(status.dma_direction.GetValue()));
        ImGui::Text("Interlace line: %s", status.interlace_even_or_odd_line ? "Odd" : "Even");

        ImGui::TreePop();
    }
    ImGui::Text("Transfer mode  %s", (mode == Mode::Command) ? "[COMMAND]" : "[DATA]");

    ImGui::Text("Draw area [x,y]");
    ImGui::Text("[%3d,%3d]------+", drawing_area_left, drawing_area_top);
    ImGui::Text("    |          | ");
    ImGui::Text("    +------[%3d,%3d]", drawing_area_right, drawing_area_bottom);

    ImGui::Text("Draw offset:  x=%d, y=%d", drawing_x_offset, drawing_y_offset);
    ImGui::Text("VRAM start:   x=%u, y=%u", display_vram_x_start, display_vram_y_start);
    ImGui::Text("Horiz. range: start=%u, end=%u", display_horizontal_start, display_horizontal_end);
    ImGui::Text("Line range:   start=%u, end=%u", display_line_start, display_line_end);

    ImGui::Text("Video mode: %s - %d BPP", status.video_mode == VideoMode::NTSC ? "NTSC" : "PAL",
                status.display_area_color_depth ? 24 : 15);
    ImGui::Text("Horizontal resolution: %u", HorizontalRes());
    ImGui::Text("Vertical resolution:   %u", VerticalRes());

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::End();
}
