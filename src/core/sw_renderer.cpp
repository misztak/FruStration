#include "sw_renderer.h"

#include <algorithm>

#include "fmt/format.h"
#include "gpu.h"
#include "macros.h"

LOG_CHANNEL(Renderer);

Renderer::Renderer() {}

void Renderer::Init(GPU* g) { gpu = g; }

static constexpr u8 Interpolate(u8 v0, u8 v1, u8 v2, s32 w0, s32 w1, s32 w2, s32 w, s32 w_half) {
    s32 v = w0 * s32(v0) + w1 * s32(v1) + w2 * s32(v2);
    s32 vd = (v + w_half) / w;
    return std::clamp(vd, 0, 255);
}

void Renderer::DrawTriangle(Vertex* v0, Vertex* v1, Vertex* v2) {
    auto weight = [](s16 vx0, s16 vy0, s16 vx1, s16 vy1, s16 vx2, s16 vy2) {
        return (s32)((vx1 - vx0) * (vy2 - vy0)) - ((vy1 - vy0) * (vx2 - vx0));
    };

    // swap if counter-clockwise
    if (weight(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y) < 0) std::swap(v1, v2);

    s16 vx0 = v0->x, vy0 = v0->y;
    s16 vx1 = v1->x, vy1 = v1->y;
    s16 vx2 = v2->x, vy2 = v2->y;

    // TODO: proper type conversion
    // bounding box
    u16 minX = std::min({vx0, vx1, vx2});
    u16 minY = std::min({vy0, vy1, vy2});
    u16 maxX = std::max({vx0, vx1, vx2});
    u16 maxY = std::max({vy0, vy1, vy2});
    // clip against drawing area
    minX = std::clamp(minX, gpu->drawing_area_left, gpu->drawing_area_right);
    minY = std::clamp(minY, gpu->drawing_area_top, gpu->drawing_area_bottom);
    maxX = std::clamp(maxX, gpu->drawing_area_left, gpu->drawing_area_right);
    maxY = std::clamp(maxY, gpu->drawing_area_top, gpu->drawing_area_bottom);

    s32 w = weight(vx0, vy0, vx1, vy1, vx2, vy2);
    s32 half_w = std::max((w / 2) - 1, 0);
    // colinear
    if (w == 0) return;

    s32 dx0 = vy1 - vy2, dy0 = vx2 - vx1;
    s32 dx1 = vy2 - vy0, dy1 = vx0 - vx2;
    s32 dx2 = vy0 - vy1, dy2 = vx1 - vx0;

    s32 w0 = weight(vx1, vy1, vx2, vy2, minX, minY);
    s32 w1 = weight(vx2, vy2, vx0, vy0, minX, minY);
    s32 w2 = weight(vx0, vy0, vx1, vy1, minX, minY);

    for (u32 py = minY; py <= maxY; py++) {
        s32 row_w0 = w0, row_w1 = w1, row_w2 = w2;
        for (u32 px = minX; px <= maxX; px++) {
            if ((row_w0 | row_w1 | row_w2) >= 0) {
                const s32 b0 = row_w0, b1 = row_w1, b2 = row_w2;
                if (draw_mode == DrawMode::MONO) {
                    // all vertices store the same color value
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = v0->c.To5551();
                }
                if (draw_mode == DrawMode::SHADED) {
                    Color color;
                    // shading
                    //color.r = Interpolate(v0.c.r, v1.c.r, v2.c.r, b0, b1, b2, w, half_w);
                    //color.g = Interpolate(v0.c.g, v1.c.g, v2.c.g, b0, b1, b2, w, half_w);
                    //color.b = Interpolate(v0.c.b, v1.c.b, v2.c.b, b0, b1, b2, w, half_w);
                    color.r = (s32(v0->c.r) * b0 + s32(v1->c.r) * b1 + s32(v2->c.r) * b2) / w / 8;
                    color.g = (s32(v0->c.g) * b0 + s32(v1->c.g) * b1 + s32(v2->c.g) * b2) / w / 8;
                    color.b = (s32(v0->c.b) * b0 + s32(v1->c.b) * b1 + s32(v2->c.b) * b2) / w / 8;
                    //LOG_DEBUG << fmt::format("Color r={}, g={}, b={} at loc ({},{})", color.r, color.g, color.b, px, py);
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = color.To5551();
                }
                if (draw_mode == DrawMode::TEXTURE) {
                    u8 tex_x = Interpolate(v0->tex_x, v1->tex_x, v2->tex_x, b0, b1, b2, w, half_w);
                    u8 tex_y = Interpolate(v0->tex_y, v1->tex_y, v2->tex_y, b0, b1, b2, w, half_w);

                    tex_x = (tex_x & ~(gpu->tex_window_x_mask * 8)) | ((gpu->tex_window_x_offset & gpu->tex_window_x_mask) * 8);
                    tex_y = (tex_y & ~(gpu->tex_window_y_mask * 8)) | ((gpu->tex_window_y_offset & gpu->tex_window_y_mask) * 8);

                    u16 base_x = gpu->status.tex_page_x_base * 64;
                    u16 base_y = gpu->status.tex_page_y_base * 256;

                    if (gpu->status.tex_page_colors == 0) {
                        u32 tx = std::min<u32>(base_x + (tex_x / 4), 1023u);
                        u32 ty = std::min<u32>(base_y + tex_y, 511u);
                        u16 palette_value = gpu->vram[tx + GPU::VRAM_WIDTH * ty];
                        u16 palette_index = (palette_value >> ((tex_x % 4) * 4)) & 0xFu;
                        u16 texel_x = std::min<u32>((palette & 0x3F) * 16 + palette_index, 1023u);
                        u16 texel_y = (palette >> 6) & 0x1FF;
                        u16 texel = gpu->vram[texel_x + GPU::VRAM_WIDTH * texel_y];
                        //LOG_DEBUG << "Texel " << texel << " from location (" << texel_x << ',' << texel_y << ')';
                        if (texel) {
                            gpu->vram[px + GPU::VRAM_WIDTH * py] = texel;
                        }
                    } else {
                        Panic("Soon (TM)");
                    }
                }
            }
            row_w0 += dx0, row_w1 += dx1, row_w2 += dx2;
        }
        w0 += dy0, w1 += dy1, w2 += dy2;
    }
}
