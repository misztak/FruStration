#include "sw_renderer.h"

#include <algorithm>
#include <tuple>

#include "fmt/format.h"

#include "debug_utils.h"
#include "gpu.h"

LOG_CHANNEL(Renderer);

Renderer::Renderer(GPU* gpu): gpu(gpu) {}

static constexpr s32 EdgeFunction(Vertex* v0, Vertex* v1, Vertex* v2) {
    return (s32) ((v1->x - v0->x) * (v2->y - v0->y) - (v1->y - v0->y) * (v2->x - v0->x));
}

static constexpr s32 EdgeFunction(Vertex* v0, Vertex* v1, s16 px, s16 py) {
    return (s32) ((v1->x - v0->x) * (py - v0->y) - (v1->y - v0->y) * (px - v0->x));
}

template <u32 draw_flags>
void Renderer::DrawTriangle() {
#define DRAW_FLAGS_SET(flags) ((draw_flags & (flags)) != 0)

    if ((draw_flags & (TEXTURED | SHADED)) == (TEXTURED | SHADED)) Panic("Unimplemented");
    // made possible by Fabian Giesen's great series of articles about software rasterizers
    // starting with:
    // https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/

    // fetch the vertices
    Vertex *v0, *v1, *v2;
    if constexpr (DRAW_FLAGS_SET(SECOND_TRIANGLE)) {
        v0 = &gpu->vertices[1], v1 = &gpu->vertices[2], v2 = &gpu->vertices[3];
    } else {
        v0 = &gpu->vertices[0], v1 = &gpu->vertices[1], v2 = &gpu->vertices[2];
    }

    // make sure vertices are oriented counter-clockwise
    // swapping any two vertices is enough
    if (EdgeFunction(v0, v1, v2) < 0) std::swap(v0, v1);

    // bounding box
    s32 minX = std::min({v0->x, v1->x, v2->x});
    s32 maxX = std::max({v0->x, v1->x, v2->x});
    s32 minY = std::min({v0->y, v1->y, v2->y});
    s32 maxY = std::max({v0->y, v1->y, v2->y});

    // clip against drawing area
    // origin is top-left
    minX = std::clamp<s32>(minX, gpu->drawing_area_left, gpu->drawing_area_right);
    maxX = std::clamp<s32>(maxX, gpu->drawing_area_left, gpu->drawing_area_right);
    minY = std::clamp<s32>(minY, gpu->drawing_area_top, gpu->drawing_area_bottom);
    maxY = std::clamp<s32>(maxY, gpu->drawing_area_top, gpu->drawing_area_bottom);

    // prepare per-pixel increments
    const s32 A01 = v0->y - v1->y, B01 = v1->x - v0->x;
    const s32 A12 = v1->y - v2->y, B12 = v2->x - v1->x;
    const s32 A20 = v2->y - v0->y, B20 = v0->x - v2->x;

    // area of the parallelogram
    const s32 area = EdgeFunction(v0, v1, v2);
    // colinear
    if (area == 0) return;

    // initial weight of the first point in the bounding box
    // weight is for the point opposite to the edge created by the first two vertices
    s32 w01_row = EdgeFunction(v0, v1, minX, minY);
    s32 w12_row = EdgeFunction(v1, v2, minX, minY);
    s32 w20_row = EdgeFunction(v2, v0, minX, minY);

    // main loop
    for (s32 py = minY; py <= maxY; py++) {
        // weights at the start of the row
        s32 w01 = w01_row;
        s32 w12 = w12_row;
        s32 w20 = w20_row;

        for (s32 px = minX; px <= maxX; px++) {
            // check if point is to the left of all edges
            // just use the sign bits
            if ((w01 | w12 | w20) >= 0) {
                // draw the pixel
                if constexpr (!DRAW_FLAGS_SET(TEXTURED | SHADED)) {
                    // mono triangle
                    // all vertices store the same color value
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = v0->c.To5551();
                }
                if constexpr (DRAW_FLAGS_SET(SHADED)) {
                    Color color;
                    // shading
                    color.r = (s32(v0->c.r) * w12 + s32(v1->c.r) * w20 + s32(v2->c.r) * w01) / area;
                    color.g = (s32(v0->c.g) * w12 + s32(v1->c.g) * w20 + s32(v2->c.g) * w01) / area;
                    color.b = (s32(v0->c.b) * w12 + s32(v1->c.b) * w20 + s32(v2->c.b) * w01) / area;
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = color.To5551();
                }
                if constexpr (DRAW_FLAGS_SET(TEXTURED)) {
                    u8 tex_x = std::clamp((v0->tex_x * w12 + v1->tex_x * w20 + v2->tex_x * w01) / area, 0, 255);
                    u8 tex_y = std::clamp((v0->tex_y * w12 + v1->tex_y * w20 + v2->tex_y * w01) / area, 0, 255);

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
                        gpu->vram[px + GPU::VRAM_WIDTH * py] = 0xFF;
                    }
                }
            }

            // increment weights by one in x direction
            w01 += A01;
            w12 += A12;
            w20 += A20;
        }

        // increment weights by one in y direction
        w01_row += B01;
        w12_row += B12;
        w20_row += B20;
    }

#undef DRAW_FLAGS_SET
}

template <u32 draw_flags>
void Renderer::Draw4PointPolygon() {
    // build 4-point polygon using two calls to DrawTriangle
    DrawTriangle<draw_flags>();
    DrawTriangle<draw_flags | SECOND_TRIANGLE>();
}

template <RectSize size>
static constexpr std::tuple<u16, u16> GetSize(Rectangle& rect) {
#define MAKE(x, y) std::make_tuple(u16(x), u16(y))

    switch (size) {
        case RectSize::ONE:      return MAKE(1, 1);
        case RectSize::EIGHT:    return MAKE(8, 8);
        case RectSize::SIXTEEN:  return MAKE(16, 16);
        case RectSize::VARIABLE: return MAKE(rect.size_x, rect.size_y);
    }

#undef MAKE
}

template <RectSize size, u32 draw_flags>
void Renderer::DrawRectangle() {
#define DRAW_FLAGS_SET(flags) ((draw_flags & (flags)) != 0)

    auto& rect = gpu->rectangle;

    auto [size_x, size_y] = GetSize<size>(rect);
    DebugAssert(size_x < 1024);
    DebugAssert(size_y < 512);

    const u16 end_x = rect.start_x + size_x;
    const u16 end_y = rect.start_y + size_y;

    for (u16 y = rect.start_y; y < end_y; y++) {
        for (u16 x = rect.start_x; x < end_x; x++) {
            if constexpr (DRAW_FLAGS_SET(TEXTURED)) {
                // textured
                gpu->vram[x + GPU::VRAM_WIDTH * y] = 0xFF;
            } else {
                // mono
                gpu->vram[x + GPU::VRAM_WIDTH * y] = rect.c.To5551();
            }
        }
    }

#undef DRAW_FLAGS_SET
}

void Renderer::Draw(u32 cmd) {
    switch ((cmd >> 24)) {
        case 0x20: DrawTriangle<OPAQUE>(); break;
        case 0x22: DrawTriangle<NO_FLAGS>(); break;
        case 0x24: DrawTriangle<TEXTURED | OPAQUE | BLENDING>(); break;
        case 0x25: DrawTriangle<TEXTURED | OPAQUE>(); break;
        case 0x26: DrawTriangle<TEXTURED | BLENDING>(); break;
        case 0x27: DrawTriangle<TEXTURED>(); break;
        case 0x30: DrawTriangle<SHADED | OPAQUE>(); break;
        case 0x32: DrawTriangle<SHADED>(); break;
        case 0x34: DrawTriangle<TEXTURED | SHADED | OPAQUE | BLENDING>(); break;
        case 0x36: DrawTriangle<TEXTURED | SHADED | BLENDING>(); break;

        case 0x28: Draw4PointPolygon<OPAQUE>(); break;
        case 0x2A: Draw4PointPolygon<NO_FLAGS>(); break;
        case 0x2C: Draw4PointPolygon<TEXTURED | OPAQUE | BLENDING>(); break;
        case 0x2D: Draw4PointPolygon<TEXTURED | OPAQUE>(); break;
        case 0x2E: Draw4PointPolygon<TEXTURED | BLENDING>(); break;
        case 0x2F: Draw4PointPolygon<TEXTURED>(); break;
        case 0x38: Draw4PointPolygon<SHADED | OPAQUE>(); break;
        case 0x3A: Draw4PointPolygon<SHADED>(); break;
        case 0x3C: Draw4PointPolygon<TEXTURED | SHADED | OPAQUE | BLENDING>(); break;
        case 0x3E: Draw4PointPolygon<TEXTURED | SHADED | BLENDING>(); break;

        case 0x60: DrawRectangle<RectSize::VARIABLE, OPAQUE>(); break;
        case 0x62: DrawRectangle<RectSize::VARIABLE, NO_FLAGS>(); break;
        case 0x68: DrawRectangle<RectSize::ONE, OPAQUE>(); break;
        case 0x6A: DrawRectangle<RectSize::ONE, NO_FLAGS>(); break;
        case 0x70: DrawRectangle<RectSize::EIGHT, OPAQUE>(); break;
        case 0x72: DrawRectangle<RectSize::EIGHT, NO_FLAGS>(); break;
        case 0x78: DrawRectangle<RectSize::SIXTEEN, OPAQUE>(); break;
        case 0x7A: DrawRectangle<RectSize::SIXTEEN, NO_FLAGS>(); break;

        case 0x64: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE | BLENDING>(); break;
        case 0x65: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE>(); break;
        case 0x66: DrawRectangle<RectSize::VARIABLE, TEXTURED | BLENDING>(); break;
        case 0x67: DrawRectangle<RectSize::VARIABLE, TEXTURED>(); break;

        // textured dots make no sense, implement if used by a game?
        //case 0x6C: DrawRectangle<RectSize::ONE, TEXTURED | OPAQUE | BLENDING>(); break;
        //case 0x6D: DrawRectangle<RectSize::ONE, TEXTURED | OPAQUE>(); break;
        //case 0x6E: DrawRectangle<RectSize::ONE, TEXTURED | BLENDING>(); break;
        //case 0x6F: DrawRectangle<RectSize::ONE, TEXTURED>(); break;

        case 0x74: DrawRectangle<RectSize::EIGHT, TEXTURED | OPAQUE | BLENDING>(); break;
        case 0x75: DrawRectangle<RectSize::EIGHT, TEXTURED | OPAQUE>(); break;
        case 0x76: DrawRectangle<RectSize::EIGHT, TEXTURED | BLENDING>(); break;
        case 0x77: DrawRectangle<RectSize::EIGHT, TEXTURED>(); break;

        case 0x7C: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE | BLENDING>(); break;
        case 0x7D: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE>(); break;
        case 0x7E: DrawRectangle<RectSize::VARIABLE, TEXTURED | BLENDING>(); break;
        case 0x7F: DrawRectangle<RectSize::VARIABLE, TEXTURED>(); break;

        default:
            Panic("Invalid draw command 0x%08X", cmd);
    }
}
