#include "gpu.h"

#include "fmt/format.h"
#include "imgui.h"
#include "macros.h"
#include "interrupt.h"

LOG_CHANNEL(GPU);

GPU::GPU() {
    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;
}

void GPU::Init(InterruptController* icontroller) {
    interrupt_controller = icontroller;

    renderer.Init(this);
}

void GPU::Step(u32 steps) {
    gpu_clock += static_cast<u32>(steps * (11.0 / 7.0));
    in_hblank = false;
    in_vblank = false;

    // finished one scanline
    if (gpu_clock >= CyclesPerScanline()) {
        gpu_clock -= CyclesPerScanline();

        scanline++;
        in_hblank = true;

        if (scanline == Scanlines()) {
            scanline = 0;
            in_vblank = true;

            // VBlank
            // update display
            vblank_cb();

            interrupt_controller->Request(IRQ::VBLANK);
        }
    }
}

void GPU::SendGP0Cmd(u32 cmd) {
    if (mode == Mode::Data) {
        if (words_remaining == 0) {
            mode = Mode::Command;
            command_counter = 0;
        } else {
            CopyRectCpuToVram(cmd);
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
        case Gp0Command::quad_mono_opaque:
            CommandAfterCount(4, std::bind(&GPU::DrawQuadMonoOpaque, this));
            break;
        case Gp0Command::quad_tex_blend_opaque:
            CommandAfterCount(8, std::bind(&GPU::DrawQuadTextureBlendOpaque, this));
            break;
        case Gp0Command::triangle_shaded_opaque:
            CommandAfterCount(5, std::bind(&GPU::DrawTriangleShadedOpaque, this));
            break;
        case Gp0Command::quad_shaded_opaque:
            CommandAfterCount(7, std::bind(&GPU::DrawQuadShadedOpaque, this));
            break;
        case Gp0Command::rect_tex_opaque:
            CommandAfterCount(3, std::bind(&GPU::DrawRectTexture, this));
            break;
        case Gp0Command::dot_mono_opaque:
            CommandAfterCount(1, [&] {
                Vertex dot;
                dot.SetPoint(command_buffer[1]);
                dot.c.SetColor(command_buffer[0]);
                vram[dot.x + VRAM_WIDTH * dot.y] = dot.c.To5551();
            });
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
        case Gp1Command::display_mode:
            status.horizontal_res_1 = cmd & 0x3;
            status.vertical_res = (cmd >> 2) & 0x1;
            status.video_mode = static_cast<VideoMode>((cmd >> 3) & 0x1);
            status.display_area_color_depth = (cmd >> 4) & 0x1;
            status.vertical_interlace = (cmd >> 5) & 0x1;
            status.horizontal_res_2 = (cmd >> 6) & 0x1;
            if ((cmd >> 7) & 0x1) Panic("Tried to set GPUSTAT.14!");
            break;
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

void GPU::DrawQuadMonoOpaque() {
    //LOG_DEBUG << "DrawQuadMonoOpaque";
    for (u8 i = 0; i < 4; i++) {
        vertices[i].SetPoint(command_buffer[i + 1]);
        vertices[i].c.SetColor(command_buffer[0]);
        vertices[i].SetTextPoint(0);
    }
    renderer.DrawTriangle<DrawMode::MONO>(&vertices[0], &vertices[1], &vertices[2]);
    renderer.DrawTriangle<DrawMode::MONO>(&vertices[1], &vertices[2], &vertices[3]);
}

void GPU::DrawQuadShadedOpaque() {
    //LOG_DEBUG << "DrawQuadShadedOpaque";
    for (u8 i = 0; i < 4; i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[(i * 2)]);
        vertices[i].SetTextPoint(0);
    }
    renderer.DrawTriangle<DrawMode::SHADED>(&vertices[0], &vertices[1], &vertices[2]);
    renderer.DrawTriangle<DrawMode::SHADED>(&vertices[1], &vertices[2], &vertices[3]);
}

void GPU::DrawQuadTextureBlendOpaque() {
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

    renderer.DrawTriangle<DrawMode::TEXTURE>(&vertices[0], &vertices[1], &vertices[2]);
    renderer.DrawTriangle<DrawMode::TEXTURE>(&vertices[1], &vertices[2], &vertices[3]);
}

void GPU::DrawTriangleShadedOpaque() {
    //LOG_DEBUG << "DrawTriangleShadedOpaque";

    for (u8 i = 0; i < 3; i++) {
        vertices[i].SetPoint(command_buffer[(i * 2) + 1]);
        vertices[i].c.SetColor(command_buffer[(i * 2)]);
        vertices[i].SetTextPoint(0);
    }
    renderer.DrawTriangle<DrawMode::SHADED>(&vertices[0], &vertices[1], &vertices[2]);
}

void GPU::DrawRectTexture() {
    //LOG_DEBUG << "DrawRectTexture";
    // Texpage gets set up separately via GP0(0xE1)
    renderer.palette = command_buffer[2] >> 16;

    rectangle.SetStart(command_buffer[1]);
    rectangle.SetTextPoint(command_buffer[2]);
    rectangle.SetSize(command_buffer[3]);
    rectangle.c.SetColor(command_buffer[0]);

    renderer.DrawRectangle();
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

        //LOG_DEBUG << "Expecting " << words_remaining << " words from CPU to GPU";
        mode = Mode::Data;
    } else if (mode == Mode::Data) {
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
    }
}

void GPU::CopyRectVramToCpu() {
    // TODO: actual implementation
    LOG_WARN << "CopyRectVramToCpu - Not implemented";
}

u32 GPU::HorizontalRes() {
    if (status.horizontal_res_2) return 368;

    switch (status.horizontal_res_1) {
        case 0: return 256;
        case 1: return 480;
        case 2: return 512;
        case 3: return 640;
        default: Panic("Invalid horizontal resolution");
    }
}

u32 GPU::VerticalRes() {
    return status.vertical_res ? 480 : 240;
}

u32 GPU::Scanlines() {
    return status.video_mode == VideoMode::NTSC ? 263 : 314;
}

u32 GPU::CyclesPerScanline() {
    return status.video_mode == VideoMode::NTSC ? 3413 : 3406;
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

void GPU::Reset() {
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
    in_hblank = in_vblank = false;

    std::fill(std::begin(vram), std::end(vram), 0);

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

    ImGui::Text("Video mode: %s", status.video_mode == VideoMode::NTSC ? "NTSC" : "PAL");
    ImGui::Text("Horizontal resolution: %u", HorizontalRes());
    ImGui::Text("Vertical resolution:   %u", VerticalRes());

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::End();
}
