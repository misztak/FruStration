#pragma once

#include <array>
#include <functional>

#include "types.h"
#include "bitfield.h"
#include "sw_renderer.h"

class InterruptController;

class GPU {
friend class Renderer;
public:
    GPU();
    void Init(InterruptController* icontroller);
    void Reset();

    void Step(u32 steps);

    u32 ReadStat();
    void SendGP0Cmd(u32 cmd);
    void SendGP1Cmd(u32 cmd);
    u16* GetVRAM();

    void DrawGpuState(bool* open);

    std::function<void()> vblank_cb = nullptr;

    bool in_hblank = false, in_vblank = false;

    u32 gpu_read = 0;

    static constexpr u32 VRAM_WIDTH = 1024;
    static constexpr u32 VRAM_HEIGHT = 512;
    static constexpr u32 VRAM_SIZE = 1024 * 512;
private:
    void DrawQuadMonoOpaque();
    void DrawQuadShadedOpaque();
    void DrawQuadTextureBlendOpaque();
    void DrawTriangleShadedOpaque();
    void DrawRectTexture();
    void CopyRectCpuToVram(u32 data = 0);
    void CopyRectVramToCpu();

    void ResetCommand();

    u32 HorizontalRes();
    u32 VerticalRes();
    u32 Scanlines();
    u32 CyclesPerScanline();

    enum class DmaDirection : u32 {
        Off = 0,
        Fifo = 1,
        CpuToGp0 = 2,
        VramToCpu = 3,
    };

    enum class VideoMode : u32 {
        NTSC, PAL
    };

    // TODO: more enums for types
    union {
        u32 value = 0;

        BitField<u32, u32, 0, 4> tex_page_x_base;
        BitField<u32, u32, 4, 1> tex_page_y_base;
        BitField<u32, u32, 5, 2> semi_transparency;
        BitField<u32, u32, 7, 2> tex_page_colors;
        BitField<u32, bool, 9, 1> dither;
        BitField<u32, bool, 10, 1> draw_enable;
        BitField<u32, bool, 11, 1> mask_enable;
        BitField<u32, bool, 12, 1> draw_pixels;
        BitField<u32, bool, 13, 1> interlace_field;
        BitField<u32, bool, 14, 1> reverse;
        BitField<u32, bool, 15, 1> tex_disable;
        BitField<u32, u32, 16, 1> horizontal_res_2;
        BitField<u32, u32, 17, 2> horizontal_res_1;
        BitField<u32, u32, 19, 1> vertical_res;
        BitField<u32, VideoMode, 20, 1> video_mode;
        BitField<u32, u32, 21, 1> display_area_color_depth;
        BitField<u32, bool, 22, 1> vertical_interlace;
        BitField<u32, bool, 23, 1> display_disabled;
        BitField<u32, bool, 24, 1> interrupt_request;
        BitField<u32, bool, 25, 1> dma_data_stat;
        BitField<u32, bool, 26, 1> can_receive_cmd_word;
        BitField<u32, bool, 27, 1> can_send_vram_to_cpu;
        BitField<u32, bool, 28, 1> can_receive_dma_block;
        BitField<u32, DmaDirection, 29, 2> dma_direction;
        BitField<u32, bool, 31, 1> interlace_line_mode;
    } status;

    bool tex_rectangle_xflip = false;
    bool tex_rectangle_yflip = false;

    u8 tex_window_x_mask = 0;
    u8 tex_window_y_mask = 0;
    u8 tex_window_x_offset = 0;
    u8 tex_window_y_offset = 0;

    u16 drawing_area_left = 0;
    u16 drawing_area_top = 0;
    u16 drawing_area_right = 0;
    u16 drawing_area_bottom = 0;

    s16 drawing_x_offset = 0;
    s16 drawing_y_offset = 0;

    u16 display_vram_x_start = 0;
    u16 display_vram_y_start = 0;
    u16 display_horizontal_start = 0;
    u16 display_horizontal_end = 0;
    u16 display_line_start = 0;
    u16 display_line_end = 0;

    enum class Mode : u32 {
        Command,
        Data,
    };

    enum class Gp0Command : u32 {
        nop = 0x00,
        clear_cache = 0x01,
        quad_mono_opaque = 0x28,
        quad_tex_blend_opaque = 0x2c,
        triangle_shaded_opaque = 0x30,
        quad_shaded_opaque = 0x38,
        rect_tex_opaque = 0x64,
        dot_mono_opaque = 0x68,
        copy_rectangle_cpu_to_vram = 0xa0,
        copy_rectangle_vram_to_cpu = 0xc0,
        draw_mode = 0xe1,
        texture_window = 0xe2,
        draw_area_top_left = 0xe3,
        draw_area_bottom_right = 0xe4,
        draw_offset = 0xe5,
        mask_setting = 0xe6,
    };

    enum class Gp1Command : u32 {
        reset = 0x00,
        cmd_buf_reset = 0x01,
        ack_gpu_interrupt = 0x02,
        display_enable = 0x03,
        dma_dir = 0x04,
        display_vram_start = 0x05,
        display_hor_range = 0x06,
        display_ver_range = 0x07,
        display_mode = 0x08,
        gpu_info = 0x10,
    };

    u32 gpu_clock = 0;
    u32 scanline = 0;

    u32 command_counter = 0;
    std::array<u32, 12> command_buffer;

    Mode mode = Mode::Command;
    u32 words_remaining = 0;

    union {
        u32 value = 0;

        BitField<u32, Gp0Command, 24, 8> gp0_op;
        BitField<u32, Gp1Command, 24, 8> gp1_op;
    } command;

    // TODO: is VRAM filled with garbage at boot?
    std::array<u16, VRAM_SIZE> vram;

    std::array<Vertex, 4> vertices;
    Rectangle rectangle = {};
    Renderer renderer;

    InterruptController* interrupt_controller = nullptr;
};