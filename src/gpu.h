#pragma once

#include "types.h"
#include "bitfield.h"

class GPU {
public:
    GPU();
    void Init();

    u32 ReadStat();
private:
    enum class DmaDirection : u32 {
        Off = 0,
        Fifo = 1,
        CpuToGp0 = 2,
        VramToCpu = 3,
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
        BitField<u32, bool, 19, 1> vertical_res;
        BitField<u32, bool, 20, 1> video_mode;
        BitField<u32, bool, 21, 1> display_area_color_depth;
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
};