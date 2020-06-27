#include "sw_renderer.h"

#include <algorithm>

#include "fmt/format.h"
#include "gpu.h"
#include "macros.h"

LOG_CHANNEL(Renderer);

Renderer::Renderer() {}

void Renderer::Init(GPU* g) { gpu = g; }

static constexpr s32 Interpolate(u8 v0, u8 v1, u8 v2, s32 w0, s32 w1, s32 w2) {
    return w0 * static_cast<s32>(v0) + w1 * static_cast<s32>(v1) + w2 * static_cast<s32>(v2);
}

u16 Renderer::GetTexel4Bit(u16 tex_x, u16 clut_index, u16 clut_x, u16 clut_y) {
    u32 offset = (clut_index >> (tex_x & 3) * 4) & 0xF;
    return gpu->vram[(clut_x + offset) + (GPU::VRAM_WIDTH * clut_y)];
}

u16 Renderer::GetTexel8Bit(u16 tex_x, u16 tex_y, u16 clut_x, u16 clut_y, u16 page_x, u16 page_y) { return 0; }
u16 Renderer::GetTexel16Bit(u16 tex_x, u16 tex_y, u16 clut_x, u16 clut_y, u16 page_x, u16 page_y) { return 0; }

void Renderer::DrawTriangle(const Vertex& v0_original, const Vertex& v1_original, const Vertex& v2_original) {
    auto orientation = [](const Vertex& v0, const Vertex& v1, const Vertex& v2) {
        return (s32)((v1.x - v0.x) * (v2.y - v0.y)) - ((v1.y - v0.y) * (v2.x - v0.x));
    };

    // can't swap const references, maybe find a cleaner way to do this
    Vertex v0 = v0_original;
    Vertex v1 = v1_original;
    Vertex v2 = v2_original;
    if (orientation(v0, v1, v2) < 0) std::swap(v1, v2);

    // v0.x += gpu->drawing_x_offset; v0.y += gpu->drawing_y_offset;
    // v1.x += gpu->drawing_x_offset; v1.y += gpu->drawing_y_offset;
    // v2.x += gpu->drawing_x_offset; v2.y += gpu->drawing_y_offset;

    s32 area = orientation(v0, v1, v2);
    if (area == 0) return;

    // bounding box
    u16 minX = std::min({v0.x, v1.x, v2.x});
    u16 minY = std::min({v0.y, v1.y, v2.y});
    u16 maxX = std::max({v0.x, v1.x, v2.x});
    u16 maxY = std::max({v0.y, v1.y, v2.y});
    // clip against drawing area
    minX = std::clamp(minX, gpu->drawing_area_left, gpu->drawing_area_right);
    minY = std::clamp(minY, gpu->drawing_area_top, gpu->drawing_area_bottom);
    maxX = std::clamp(maxX, gpu->drawing_area_left, gpu->drawing_area_right);
    maxY = std::clamp(maxY, gpu->drawing_area_top, gpu->drawing_area_bottom);

#if 0
    minX = gpu->drawing_area_left;
    minY = gpu->drawing_area_top;
    maxX = gpu->drawing_area_right;
    maxY = gpu->drawing_area_bottom;
#endif

    // printf("Drawing triangle in bounding box (%u, %u) - (%u, %u), drawing area is (%u, %u) - (%u, %u)\n",
    //        minX, minY, maxX, maxY, gpu->drawing_area_left, gpu->drawing_area_top,
    //        gpu->drawing_area_right, gpu->drawing_area_bottom);

    Vertex p(0, 0, Color(0));
    for (p.y = minY; p.y <= maxY; p.y++) {
        for (p.x = minX; p.x <= maxX; p.x++) {
            s32 w0 = orientation(v1, v2, p);
            s32 w1 = orientation(v2, v0, p);
            s32 w2 = orientation(v0, v1, p);

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                switch (draw_mode) {
                    case DrawMode::CLEAR:
                        Panic("Forgot to set draw mode!");
                        break;
                    case DrawMode::MONO:
                        // all vertices store the same color value
                        gpu->vram[p.x + GPU::VRAM_WIDTH * p.y] = v0.c.To5551();
                        break;
                    case DrawMode::SHADED: {
                        Color color(0);
                        // shading
                        color.r = Interpolate(v0.c.r, v1.c.r, v2.c.r, w0, w1, w2) / area / 8;
                        color.g = Interpolate(v0.c.g, v1.c.g, v2.c.g, w0, w1, w2) / area / 8;
                        color.b = Interpolate(v0.c.b, v1.c.b, v2.c.b, w0, w1, w2) / area / 8;

                        gpu->vram[p.x + GPU::VRAM_WIDTH * p.y] = color.To5551();
                        break;
                    }
                    case DrawMode::TEXTURE: {
                        const u16 tex_x = (w0 * v0.tex_x + w1 * v1.tex_x + w2 * v2.tex_x) / area;
                        const u16 tex_y = (w0 * v0.tex_y + w1 * v1.tex_y + w2 * v2.tex_y) / area;

                        u16 base_x = gpu->status.tex_page_x_base * 64;
                        u16 base_y = gpu->status.tex_page_y_base * 256;
                        u16 tex_y_abs = base_y + tex_y;

                        // Color color;
                        u16 clut_x = (palette & 0x3F) * 16;
                        u16 clut_y = (palette >> 6) & 0x1FF;

                        switch (gpu->status.tex_page_colors) {
                            case 0: {
                                u16 tex_x_abs = base_x + (tex_x / 4);
                                u16 clut_index = gpu->vram[tex_x_abs + GPU::VRAM_WIDTH * tex_y_abs];
                                //u16 texel = GetTexel4Bit(tex_x, clut_index, clut_x, clut_y);
                                //Color c(clut_index);
                                //LOG_DEBUG << fmt::format(
                                //    "x={}, y={}, tex_x={}, tex_y={}, base_x={}, base_y={}, tex_x_abs={}, "
                                //    "tex_t_abs={}, clut_x={}, clut_y={}, clut_index={}",
                                //    p.x, p.y, tex_x, tex_y, base_x, base_y, tex_x_abs, tex_y_abs, clut_x, clut_y,
                                //    clut_index);
                                gpu->vram[p.x + GPU::VRAM_WIDTH * p.y] = clut_index;
                                break;
                            }
                            default:
                                Panic("Soon (TM)");
                        }

                        break;
                    }
                }
            }
        }
    }
}
