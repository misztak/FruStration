#include "gpu.h"

#include "fmt/format.h"
#include "imgui.h"
#include "macros.h"
#include "interrupt.h"
#include "scheduler.h"
#include "timer.h"

LOG_CHANNEL(GPU);

GPU::GPU(): vram(VRAM_SIZE, 0), output(VRAM_SIZE * 2, 0) {
    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;

    Scheduler::AddComponent(
        Component::Type::GPU,
        [this](u32 cycles) { StepTmp(cycles); },
        [this] { return CyclesUntilNextEvent(); }
    );
}

void GPU::Init(TimerController* tmr, InterruptController* icontroller) {
    timer_controller = tmr;
    interrupt_controller = icontroller;

    renderer.Init(this);
}

void GPU::StepTmp(u32 cpu_cycles) {
    DebugAssert(cpu_cycles <= cycles_until_next_event);

    auto [gpu_cycles, dots] = GpuCyclesAndDots(cpu_cycles);

    accumulated_dots += dots;

    const float dots_per_scanline = DotsPerScanline();
    while (accumulated_dots >= dots_per_scanline) {
        accumulated_dots -= dots_per_scanline;
        scanline = (scanline + 1) % Scanlines();
        // TODO: even/odd bit
    }

    auto& dot_timer = timer_controller->timers[0];
    if (!dot_timer.IsUsingSystemClock()) {
        static float dotclock_dots = 0;
        dotclock_dots += dots;
        dot_timer.Increment(static_cast<u32>(dotclock_dots));
        dotclock_dots = std::fmod(dotclock_dots, 1.0f);
    }

    const bool currently_in_hblank = IsGpuInHblank();

    if (dot_timer.mode.sync_enabled) {
        dot_timer.UpdateOnBlankFlip(TMR0, currently_in_hblank);
    }

    auto& hblank_timer = timer_controller->timers[1];
    if (!hblank_timer.IsUsingSystemClock()) {
        const u32 lines = static_cast<u32>(dots / dots_per_scanline);
        hblank_timer.Increment(static_cast<u32>(currently_in_hblank && !was_in_vblank) + lines);
    }

    was_in_hblank = currently_in_hblank;

    const bool currently_in_vblank = IsGpuInVblank();

    if (hblank_timer.mode.sync_enabled) {
        hblank_timer.UpdateOnBlankFlip(TMR1, currently_in_vblank);
    }

    if (!was_in_vblank && currently_in_vblank) {
        interrupt_controller->Request(IRQ::VBLANK);

        draw_frame = true;
        //LOG_DEBUG << "VBLANK";

        // TODO: interlaced even/odd stuff
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

    auto& dot_timer = timer_controller->timers[0];
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
    const float cycles_until_vblank_flip = lines_until_vblank_flip * GpuCyclesPerScanline() - accumulated_dots / dots_per_cycle;
    MinCycles(cycles_until_vblank_flip);

    auto& hblank_timer = timer_controller->timers[1];
    if (!hblank_timer.paused && !hblank_timer.IsUsingSystemClock()) {
        const float cycles_until_hblank = ((accumulated_dots < horizontal_res ? horizontal_res : dots_per_scanline + horizontal_res) - accumulated_dots) / dots_per_cycle;
        const float cycles_until_irq = hblank_timer.CyclesUntilNextIRQ() * GpuCyclesPerScanline() - cycles_until_hblank;
        MinCycles(cycles_until_irq);
    }

    cycles_until_next_event = static_cast<u32>(std::ceil(gpu_cycles_until_next_event / (11.0 / 7.0)));

    return cycles_until_next_event;
}

void GPU::SendGP0Cmd(u32 cmd) {
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

    auto CommandAfterCount = [&](u32 count, auto function) {
        if (command_counter == count) {
            function();
            command_counter = 0;
        } else {
            command_counter++;
        }
    };

    switch (command.gp0_op) {
        case Gp0Command::nop:
            break;
        case Gp0Command::clear_cache:
            // TODO: implement me
            break;
        case Gp0Command::fill_vram:
            CommandAfterCount(2, [&]{
                Rectangle r;
                r.c.SetColor(command_buffer[0]);
                r.SetStart(command_buffer[1]);
                r.SetSize(command_buffer[2]);
                for (u32 y = r.start_y; y < r.start_y + r.size_y; y++) {
                    for (u32 x = r.start_x; x < r.start_x + r.size_x; x++) {
                        vram[y * VRAM_WIDTH + x] = r.c.To5551();
                    }
                }
            });
            break;
        case Gp0Command::quad_mono:
            CommandAfterCount(4, std::bind(&GPU::DrawQuadMono, this));
            break;
        case Gp0Command::quad_textured:
            CommandAfterCount(8, std::bind(&GPU::DrawQuadTextured, this));
            break;
        case Gp0Command::triangle_shaded:
            CommandAfterCount(5, std::bind(&GPU::DrawTriangleShaded, this));
            break;
        case Gp0Command::quad_shaded:
            CommandAfterCount(7, std::bind(&GPU::DrawQuadShaded, this));
            break;
        case Gp0Command::rectangle_textured:
            CommandAfterCount(3, std::bind(&GPU::DrawRectangleTexture, this));
            break;
        case Gp0Command::rectangle_mono:
            CommandAfterCount(1, std::bind(&GPU::DrawRectangleMono, this));
            break;
        case Gp0Command::copy_rectangle_cpu_to_vram:
            CommandAfterCount(2, std::bind(&GPU::CopyRectCpuToVram, this, 0));
            break;
        case Gp0Command::copy_rectangle_vram_to_cpu:
            CommandAfterCount(2, std::bind(&GPU::CopyRectVramToCpu, this));
            break;
        case Gp0Command::draw_mode:
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
        case Gp0Command::texture_window:
            tex_window_x_mask = cmd & 0x1F;
            tex_window_y_mask = (cmd >> 5) & 0x1F;
            tex_window_x_offset = (cmd >> 10) & 0x1F;
            tex_window_y_offset = (cmd >> 15) & 0x1F;
            break;
        case Gp0Command::draw_area_top_left:
            drawing_area_top = static_cast<u16>((cmd >> 10) & 0x1FF);
            drawing_area_left = static_cast<u16>(cmd & 0x3FF);
            //LOG_DEBUG << "Set draw area top=" << drawing_area_top << ", left=" << drawing_area_left;
            break;
        case Gp0Command::draw_area_bottom_right:
            drawing_area_bottom = static_cast<u16>((cmd >> 10) & 0x1FF);
            drawing_area_right = static_cast<u16>(cmd & 0x3FF);
            //LOG_DEBUG << "Set draw area bottom=" << drawing_area_bottom << ", right=" << drawing_area_right;
            break;
        case Gp0Command::draw_offset: {
            const u16 x = static_cast<u16>(cmd & 0x7FF);
            const u16 y = static_cast<u16>((cmd >> 11) & 0x7FF);
            drawing_x_offset = static_cast<s16>(x << 5) >> 5;
            drawing_y_offset = static_cast<s16>(y << 5) >> 5;
            break;
        }
        case Gp0Command::mask_setting:
            status.mask_enable = cmd & 0x1;
            status.draw_pixels = (cmd >> 1) & 0x1;
            break;
        default:
            Panic("Unimplemented GP0 command 0x%08X", cmd);
    }
}

void GPU::SendGP1Cmd(u32 cmd) {
    //LOG_DEBUG << fmt::format("GPU received GP1 command 0x{:08X}", cmd);
    command.value = cmd;

    switch (command.gp1_op) {
        case Gp1Command::reset:
            ResetCommand();
            break;
        case Gp1Command::cmd_buf_reset:
            LOG_DEBUG << "Reset command buffer [Unimplemented]";
            break;
        case Gp1Command::ack_gpu_interrupt:
            LOG_DEBUG << "ACK GPU Interrupt [Unimplemented]";
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
        case Gp1Command::display_mode: {
            GpuStatus new_status;
            new_status.value = status.value;

            new_status.horizontal_res_1 = cmd & 0x3;
            new_status.vertical_res = (cmd >> 2) & 0x1;
            new_status.video_mode = static_cast<VideoMode>((cmd >> 3) & 0x1);
            new_status.display_area_color_depth = (cmd >> 4) & 0x1;
            new_status.vertical_interlace = (cmd >> 5) & 0x1;
            new_status.horizontal_res_2 = (cmd >> 6) & 0x1;
            if ((cmd >> 7) & 0x1) Panic("Tried to set GPUSTAT.14!");

            if (new_status.value != status.value) {
                Scheduler::ForceUpdate();
                status.value = new_status.value;
                Scheduler::RecalculateNextEvent();
            }

            break;
        }
        case Gp1Command::gpu_info:
            // we pretend to have the older 160pin GPU
            // TODO: differences between old and new GPU?
            // TODO: remaining options
            switch (cmd & 0xFFFFFF) {
                case 7: break;
                default: Panic("Unimplemented GPU info option %u", cmd & 0xFFFFFF);
            }
            break;
        default:
            Panic("Unimplemented GP1 command 0x%08X", cmd);
    }
}

void GPU::DrawQuadMono() {
    //LOG_DEBUG << "DrawQuadMonoOpaque";
    for (u8 i = 0; i < 4; i++) {
        vertices[i].SetPoint(command_buffer[i + 1]);
        vertices[i].c.SetColor(command_buffer[0]);
        vertices[i].SetTextPoint(0);
    }
    renderer.Draw(command_buffer[0]);
}

void GPU::DrawQuadShaded() {
    //LOG_DEBUG << "DrawQuadShadedOpaque";
    for (u8 i = 0; i < 4; i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[(i * 2)]);
        vertices[i].SetTextPoint(0);
    }
    renderer.Draw(command_buffer[0]);
}

void GPU::DrawQuadTextured() {
    //LOG_DEBUG << "DrawQuadTextureBlendOpaque";
    renderer.palette = command_buffer[2] >> 16;

    u16 texpage_attribute = command_buffer[4] >> 16;
    status.tex_page_x_base = texpage_attribute & 0xF;
    status.tex_page_y_base = (texpage_attribute >> 4) & 0x1;
    status.semi_transparency = (texpage_attribute >> 5) & 0x3;
    status.tex_page_colors = (texpage_attribute >> 7) & 0x3;
    status.tex_disable = (texpage_attribute >> 11) & 0x1;

    for (u8 i = 0; i < 4; i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[0]);
        vertices[i].SetTextPoint(command_buffer[(i * 2) + 2]);
        //printf("(x=%d, y=%d) ", vertices[i].x, vertices[i].y);
    }
    //printf("\n");

    renderer.Draw(command_buffer[0]);
}

void GPU::DrawTriangleShaded() {
    //LOG_DEBUG << "DrawTriangleShadedOpaque";

    for (u8 i = 0; i < 3; i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[(i * 2)]);
        vertices[i].SetTextPoint(0);
    }
    renderer.Draw(command_buffer[0]);
}

void GPU::DrawRectangleMono() {
    //LOG_DEBUG << "DrawRectangleMono";

    rectangle.c.SetColor(command_buffer[0]);
    rectangle.SetStart(command_buffer[1]);
    rectangle.SetSize(command_buffer[2]);

    renderer.Draw(command_buffer[0]);
}

void GPU::DrawRectangleTexture() {
    //LOG_DEBUG << "DrawRectangleTexture";

    rectangle.c.SetColor(command_buffer[0]);
    rectangle.SetStart(command_buffer[1]);
    rectangle.SetTextPoint(command_buffer[2]);
    rectangle.SetSize(command_buffer[3]);

    // Texpage gets set up separately via GP0(0xE1)
    renderer.palette = command_buffer[2] >> 16;

    renderer.Draw(command_buffer[0]);
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

        // determine the end of a line
        x_pos_max = x_pos + width;
        Assert(x_pos_max < VRAM_WIDTH);

        mode = Mode::DataFromCPU;
        //LOG_DEBUG << fmt::format("CopyCPUtoVram: {} words from (x={}, y={}) to (x={}, y={})",
        //                         words_remaining, x_pos, y_pos, x_pos + width - 1, y_pos + height - 1);
    } else if (mode == Mode::DataFromCPU) {
        // first halfword
        vram[x_pos + VRAM_WIDTH * y_pos] = data & 0xFFFF;
        // increment and check for overflow
        auto increment = [&] {
            x_pos++;
            if (x_pos >= x_pos_max) {
                y_pos = (y_pos + 1) % 512;
                x_pos = command_buffer[1] & 0x3FF;
            }
        };
        increment();

        // second halfword
        vram[x_pos + VRAM_WIDTH * y_pos] = data >> 16;
        increment();
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

        // determine the end of a line
        x_pos_max = x_pos + width;
        Assert(x_pos_max < VRAM_WIDTH);
        mode = Mode::DataToCPU;
        LOG_DEBUG << fmt::format("CopyVramToCPU: {} words from (x={}, y={}) to (x={}, y={})",
                                 words_remaining, x_pos, y_pos, x_pos + width - 1, y_pos + height - 1);
    } else if (mode == Mode::DataToCPU) {
        // first halfword
        u32 word1 = vram[x_pos + VRAM_WIDTH * y_pos];
        // increment and check for overflow
        auto increment = [&] {
            x_pos++;
            if (x_pos >= x_pos_max) {
                y_pos = (y_pos + 1) % 512;
                x_pos = command_buffer[1] & 0x3FF;
            }
        };
        increment();

        // second halfword
        u32 word2 = vram[x_pos + VRAM_WIDTH * y_pos];
        increment();

        gpu_read = (word2 << 16) | word1;
    } else {
        Panic("Invalid GPU transfer mode during CopyRectVramToCpu");
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
    u32 hack = status.value;
    hack &= ~(1u << 19);
    return hack;
    //return status.value;
    //return 0b01011110100000000000000000000000;
}

u16* GPU::GetVRAM() {
    return vram.data();
}

u8* GPU::GetVideoOutput() {
    // copy display area from vram to output
    const u32 count = status.display_area_color_depth ? (640 * 3) : (640 * 2);
    for (u32 y = 0; y < 480; y++) {
        std::memcpy(output.data() + count * y, (u8*)(vram.data() + VRAM_WIDTH * y), count);
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
    renderer.palette = 0;
}

void GPU::DrawGpuState(bool* open) {
    ImGui::Begin("GPU State", open);

    ImGui::Columns(2);
    ImGui::Text("Command buffer");
    ImGui::Separator();
    const ImVec4 white(1.0, 1.0, 1.0, 1.0);
    const ImVec4 grey(0.5, 0.5, 0.5, 1.0);
    for (u32 i=0; i<12; i++) {
        ImGui::TextColored((i > command_counter) ? grey : white, " %-2d  %08X", i, command_buffer[i]);
        if (i == command_counter) {
            ImGui::SameLine();
            ImGui::Text("  <--- cmd end");
        }
    }
    ImGui::NextColumn();
    if (ImGui::TreeNode("__gpustat_node", "GPUSTAT   %08X", status.value)) {
        ImGui::TreePop();
    }
    ImGui::Text("Transfer mode  %s", (mode == Mode::Command) ? "COMMAND" : "DATA");

    ImGui::Text("Draw area [x,y]");
    ImGui::Text("[%3d,%3d]------+", drawing_area_left, drawing_area_top);
    ImGui::Text("    |          | ");
    ImGui::Text("    +------[%3d,%3d]", drawing_area_right, drawing_area_bottom);

    ImGui::Text("Draw offset:  x=%d, y=%d", drawing_x_offset, drawing_y_offset);
    ImGui::Text("VRAM start:   x=%u, y=%u", display_vram_x_start, display_vram_y_start);
    ImGui::Text("Horiz. range: start=%u, end=%u", display_horizontal_start, display_horizontal_end);
    ImGui::Text("Line range:   start=%u, end=%u", display_line_start, display_line_end);

    ImGui::Text("Video mode: %s - %d BPP", status.video_mode == VideoMode::NTSC ? "NTSC" : "PAL", status.display_area_color_depth ? 24 : 15);
    ImGui::Text("Horizontal resolution: %u", HorizontalRes());
    ImGui::Text("Vertical resolution:   %u", VerticalRes());

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::End();
}
