#pragma once

#include "util/types.h"

struct Stats {
    // renderer
    u64 frame_triangle_draw_count;
    u64 frame_rectangle_draw_count;
    u64 frame_line_draw_count;

    void ResetPerFrameStats() {
        frame_triangle_draw_count = 0;
        frame_rectangle_draw_count = 0;
        frame_line_draw_count = 0;
    }
};
