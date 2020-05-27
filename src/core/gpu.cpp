#include "gpu.h"

#include "imgui.h"

GPU::GPU() {
    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;
}

void GPU::Init() {
    renderer.Init(this);
}

void GPU::SendGP0Cmd(u32 cmd) {
    if (mode == Mode::Data) {
        if (words_remaining == 0) {
            mode = Mode::Command;
            command_counter = 0;
        } else {
            CopyRectCpuToVram(cmd);
            words_remaining--;
        }
        return;
    }

    if (command_counter == 0) {
        //printf("GPU received GP0 command 0x%08X\n", cmd);
        command.value = cmd;
    }
    DebugAssert(command_counter < 12);
    command_buffer[command_counter] = cmd;

    // clear the draw flags
    renderer.draw_flags = Renderer::DRAW_FLAG_CLEAR;

    switch (command.gp0_op) {
        case Gp0Command::nop:
            break;
        case Gp0Command::clear_cache:
            // TODO: implement me
            break;
        case Gp0Command::quad_mono_opaque:
            if (command_counter == 4) {
                DrawQuadMonoOpaque();
                command_counter = 0;
            } else {
                command_counter++;
            }
            break;
        case Gp0Command::quad_tex_blend_opaque:
            if (command_counter == 8) {
                DrawQuadTextureBlendOpaque();
                command_counter = 0;
            } else {
                command_counter++;
            }
            break;
        case Gp0Command::triangle_shaded_opaque:
            if (command_counter == 5) {
                DrawTriangleShadedOpaque();
                command_counter = 0;
            } else {
                command_counter++;
            }
            break;
        case Gp0Command::quad_shaded_opaque:
            if (command_counter == 7) {
                DrawQuadShadedOpaque();
                command_counter = 0;
            } else {
                command_counter++;
            }
            break;
        case Gp0Command::copy_rectangle_cpu_to_vram:
            if (command_counter == 2) {
                CopyRectCpuToVram();
                //command_counter = 0;
            } else {
                command_counter++;
            }
            break;
        case Gp0Command::copy_rectangle_vram_to_cpu:
            if (command_counter == 2) {
                CopyRectVramToCpu();
                command_counter = 0;
            } else {
                command_counter++;
            }
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
            drawing_area_top = static_cast<u16>((cmd >> 10) & 0x3FF);
            drawing_area_left = static_cast<u16>(cmd & 0x3FF);
            break;
        case Gp0Command::draw_area_bottom_right:
            drawing_area_bottom = static_cast<u16>((cmd >> 10) & 0x3FF);
            drawing_area_right = static_cast<u16>(cmd & 0x3FF);
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
    //printf("GPU received GP1 command 0x%08X\n", cmd);
    command.value = cmd;

    switch (command.gp1_op) {
        case Gp1Command::reset:
            ResetCommand();
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
            status.video_mode = (cmd >> 3) & 0x1;
            status.display_area_color_depth = (cmd >> 4) & 0x1;
            status.vertical_interlace = (cmd >> 5) & 0x1;
            status.horizontal_res_2 = (cmd >> 6) & 0x1;
            if ((cmd >> 7) & 0x1) Panic("Tried to set GPUSTAT.14!");
            break;
        default:
            Panic("Unimplemented GP1 command 0x%08X", cmd);
    }
}

void GPU::DrawQuadMonoOpaque() {
    printf("DrawQuadMonoOpaque\n");
    renderer.draw_flags |= Renderer::DRAW_FLAG_MONO;
    Color mono(command_buffer[0]);
    renderer.DrawTriangle(Vertex(command_buffer[1], mono),
                          Vertex(command_buffer[2], mono),
                          Vertex(command_buffer[3], mono));

    renderer.DrawTriangle(Vertex(command_buffer[2], mono),
                          Vertex(command_buffer[3], mono),
                          Vertex(command_buffer[4], mono));
}

void GPU::DrawQuadShadedOpaque() {
    printf("DrawQuadShadedOpaque\n");
    renderer.draw_flags |= Renderer::DRAW_FLAG_SHADED;
    renderer.DrawTriangle(Vertex(command_buffer[1], Color(command_buffer[0])),
                          Vertex(command_buffer[3], Color(command_buffer[2])),
                          Vertex(command_buffer[5], Color(command_buffer[4])));

    renderer.DrawTriangle(Vertex(command_buffer[3], Color(command_buffer[2])),
                          Vertex(command_buffer[5], Color(command_buffer[4])),
                          Vertex(command_buffer[7], Color(command_buffer[6])));
}

void GPU::DrawQuadTextureBlendOpaque() {
    printf("DrawQuadTextureBlendOpaque\n");
    // placeholder: only load a red mono quad for now
    renderer.draw_flags |= Renderer::DRAW_FLAG_MONO;
    Color mono(0xFF);
    renderer.DrawTriangle(Vertex(command_buffer[1], mono),
                          Vertex(command_buffer[3], mono),
                          Vertex(command_buffer[5], mono));

    renderer.DrawTriangle(Vertex(command_buffer[3], mono),
                          Vertex(command_buffer[5], mono),
                          Vertex(command_buffer[7], mono));
}

void GPU::DrawTriangleShadedOpaque() {
    printf("DrawTriangleShadedOpaque\n");
    renderer.draw_flags |= Renderer::DRAW_FLAG_SHADED;
    renderer.DrawTriangle(Vertex(command_buffer[1], Color(command_buffer[0])),
                          Vertex(command_buffer[3], Color(command_buffer[2])),
                          Vertex(command_buffer[5], Color(command_buffer[4])));
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

        printf("Expecting %u words from CPU to GPU\n", words_remaining);
        mode = Mode::Data;
    } else if (mode == Mode::Data) {
        // first halfword
        vram[x_pos + VRAM_WIDTH * y_pos] = data & 0xFFFF;
        // increment and check for overflow
        auto increment = [&] {
            x_pos++;
            if (x_pos >= x_pos_max) {
                y_pos++;
                Assert(y_pos < 512);
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
    printf("CopyRectVramToCpu - Not implemented\n");
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
    status.video_mode = 0;
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
}

u16* GPU::GetVRAM() {
    return vram.data();
}

void GPU::Reset() {
    status.value = 0;

    drawing_area_left = 0;
    drawing_area_top = 0;
    drawing_area_right = 0;
    drawing_area_bottom = 0;
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

    std::fill(std::begin(vram), std::end(vram), 0);

    renderer.draw_flags = Renderer::DRAW_FLAG_CLEAR;
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
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::End();
}
