#include "sw_renderer.h"

#include <algorithm>

#include "gpu.h"

Renderer::Renderer() {}

void Renderer::Init(GPU* g) { gpu = g; }

static constexpr s32 Interpolate(u8 v0, u8 v1, u8 v2, s32 w0, s32 w1, s32 w2) {
    return w0 * static_cast<s32>(v0) + w1 * static_cast<s32>(v1) + w2 * static_cast<s32>(v2);
}

void Renderer::DrawTriangle(const Vertex& v0, const Vertex& v1_original, const Vertex& v2_original) {
    auto orientation = [](const Vertex& v0, const Vertex& v1, const Vertex& v2) {
        return (s32)((v1.x - v0.x) * (v2.y - v0.y)) - ((v1.y - v0.y) * (v2.x - v0.x));
    };

    // can't swap const references, maybe find a cleaner way to do this
    Vertex v1 = v1_original;
    Vertex v2 = v2_original;
    if (orientation(v0, v1, v2) < 0) std::swap(v1, v2);

    s32 area = orientation(v0, v1, v2);
    if (area == 0) return;

    // bounding box
    u16 minX = std::min({v0.x, v1.x, v2.x});
    u16 minY = std::min({v0.y, v1.y, v2.y});
    u16 maxX = std::max({v0.x, v1.x, v2.x});
    u16 maxY = std::max({v0.y, v1.y, v2.y});
    // clip against drawing area
    minX = std::max(minX, gpu->drawing_area_left);
    minY = std::max(minY, gpu->drawing_area_top);
    maxX = std::min(maxX, gpu->drawing_area_right);
    maxY = std::min(maxY, gpu->drawing_area_bottom);

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
                if (draw_flags & DRAW_FLAG_MONO) {
                    // all vertices store the same color value
                    gpu->vram[p.x + GPU::VRAM_WIDTH * p.y] = v0.c.To5551();
                }

                if (draw_flags & DRAW_FLAG_SHADED) {
                    Color color(0);
                    // shading
                    color.r = Interpolate(v0.c.r, v1.c.r, v2.c.r, w0, w1, w2) / area / 8;
                    color.g = Interpolate(v0.c.g, v1.c.g, v2.c.g, w0, w1, w2) / area / 8;
                    color.b = Interpolate(v0.c.b, v1.c.b, v2.c.b, w0, w1, w2) / area / 8;

                    gpu->vram[p.x + GPU::VRAM_WIDTH * p.y] = color.To5551();
                }

            }
        }
    }
}
