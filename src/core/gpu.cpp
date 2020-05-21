#include "gpu.h"

GPU::GPU() {
    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;
}
void GPU::Init() {
    renderer.Init();
}

void GPU::SendGP0Cmd(u32 cmd) {
    if (send_mode == SendMode::ImageLoad) {
        if (words_remaining == 0) {
            send_mode = SendMode::Command;
            command_counter = 0;
        } else {
            //printf("GPU received GP0 image data 0x%08X\n", cmd);
            words_remaining--;
        }
        return;
    }

    if (command_counter == 0) {
        printf("GPU received GP0 command 0x%08X\n", cmd);
        command.value = cmd;
    }
    DebugAssert(command_counter < 12);
    command_buffer[command_counter] = cmd;

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

            frame_done_hack = true;
            break;
        }
        case Gp0Command::mask_setting:
            status.mask_enable = cmd & 0x1;
            status.draw_pixels = (cmd >> 1) & 0x1;
            break;
        default:
            Panic("Unimplemented GP0 command!");
    }
}

void GPU::SendGP1Cmd(u32 cmd) {
    printf("GPU received GP1 command 0x%08X\n", cmd);
    command.value = cmd;

    switch (command.gp1_op) {
        case Gp1Command::reset:
            Reset();
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
            Panic("Unimplemented GP1 command!");
    }
}

void GPU::DrawQuadMonoOpaque() {
    printf("DrawQuadMonoOpaque\n");
}

void GPU::DrawQuadShadedOpaque() {
    printf("DrawQuadShadedOpaque\n");
}

void GPU::DrawQuadTextureBlendOpaque() {
    printf("DrawQuadTextureBlendOpaque\n");
}

void GPU::DrawTriangleShadedOpaque() {
    printf("DrawTriangleShadedOpaque\n");
    renderer.PushTriangle({command_buffer[1], command_buffer[3], command_buffer[5]},
                          {command_buffer[0], command_buffer[2], command_buffer[4]});
}

void GPU::CopyRectCpuToVram() {
    const u32 resolution = command_buffer[2];
    const u32 width = resolution & 0xFFFF;
    const u32 height = resolution >> 16;
    u32 img_size = width * height;
    // round up
    img_size = (img_size + 1) & ~0x1;
    // the GPU uses 16bit pixels but receives them in 32bit packets
    words_remaining = img_size / 2;
    printf("Expecting %u words from CPU to GPU\n", words_remaining);
    send_mode = SendMode::ImageLoad;
}

void GPU::CopyRectVramToCpu() {
    // TODO: actual implementation
    printf("CopyRectVramToCpu - Not implemented\n");
}

void GPU::Draw() {
    renderer.Draw();
}

void GPU::Reset() {
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
