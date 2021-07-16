#include "sw_renderer.h"

#include <algorithm>
#include <tuple>
#include <immintrin.h>

#include "fmt/format.h"
#include "gpu.h"
#include "macros.h"

LOG_CHANNEL(Renderer);

Renderer::Renderer() {}

void Renderer::Init(GPU* g) { gpu = g; }

static constexpr s32 EdgeFunction(Vertex* v0, Vertex* v1, Vertex* v2) {
    return (s32) ((v1->x - v0->x) * (v2->y - v0->y) - (v1->y - v0->y) * (v2->x - v0->x));
}

static constexpr s32 EdgeFunction(Vertex* v0, Vertex* v1, s16 px, s16 py) {
    return (s32) ((v1->x - v0->x) * (py - v0->y) - (v1->y - v0->y) * (px - v0->x));
}

template <DrawMode mode>
void Renderer::DrawTriangle(Vertex* v0, Vertex* v1, Vertex* v2) {
    // made possible by Fabian Giesen's great series of articles about software rasterizers
    // starting with:
    // https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/

    // make sure vertices are oriented counter-clockwise
    // swapping any two vertices is enough
    if (EdgeFunction(v0, v1, v2) < 0) std::swap(v0, v1);

    // bounding box
    u16 minX = std::min({v0->x, v1->x, v2->x});
    u16 maxX = std::max({v0->x, v1->x, v2->x});
    u16 minY = std::min({v0->y, v1->y, v2->y});
    u16 maxY = std::max({v0->y, v1->y, v2->y});

    // clip against drawing area
    // origin is top-left
    minX = std::clamp(minX, gpu->drawing_area_left, gpu->drawing_area_right);
    maxX = std::clamp(maxX, gpu->drawing_area_left, gpu->drawing_area_right);
    minY = std::clamp(minY, gpu->drawing_area_top, gpu->drawing_area_bottom);
    maxY = std::clamp(maxY, gpu->drawing_area_top, gpu->drawing_area_bottom);

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
    for (u16 py = minY; py <= maxY; py++) {
        // weights at the start of the row
        s32 w01 = w01_row;
        s32 w12 = w12_row;
        s32 w20 = w20_row;

        for (u16 px = minX; px <= maxX; px++) {
            // check if point is to the left of all edges
            // just use the sign bits
            if ((w01 | w12 | w20) >= 0) {
                // draw the pixel
                if constexpr (mode == DrawMode::MONO) {
                    // all vertices store the same color value
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = v0->c.To5551();
                }
                if constexpr (mode == DrawMode::SHADED) {
                    Color color;
                    // shading
                    color.r = (s32(v0->c.r) * w12 + s32(v1->c.r) * w20 + s32(v2->c.r) * w01) / area / 8;
                    color.g = (s32(v0->c.g) * w12 + s32(v1->c.g) * w20 + s32(v2->c.g) * w01) / area / 8;
                    color.b = (s32(v0->c.b) * w12 + s32(v1->c.b) * w20 + s32(v2->c.b) * w01) / area / 8;
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = color.To5551();
                }
                if constexpr (mode == DrawMode::TEXTURE) {
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
                            gpu->vram[px + GPU::VRAM_WIDTH * py] = 0xFF;
                        }
                    } else {
                        Panic("Soon (TM)");
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
}

template <DrawMode mode>
void Renderer::DrawTriangleAVX2(Vertex *v0, Vertex *v1, Vertex *v2, u16 minX, u16 maxX, u16 minY, u16 maxY) {}

void Renderer::DrawRectangleWith(Rectangle::DrawMode mode, Rectangle::Size size, Rectangle::Opacity opacity,
                                  Rectangle::Texture texture) {
#define FN(SIZE, OPACITY) \
    &Renderer::DrawRectangleMono<Rectangle::Size::SIZE, Rectangle::Opacity::OPACITY>

    // inspired by Duckstation's approach for selecting triangle draw functions
    using DrawRectangleFunctions = void (Renderer::*)();
    static constexpr DrawRectangleFunctions mono_functions[4][2] = {
        {FN(ONE, OPAQUE),       FN(ONE, SEMI_TRANSPARENT)},
        {FN(EIGHT, OPAQUE),     FN(EIGHT, SEMI_TRANSPARENT)},
        {FN(SIXTEEN, OPAQUE),   FN(EIGHT, SEMI_TRANSPARENT)},
        {FN(VARIABLE, OPAQUE),  FN(VARIABLE, SEMI_TRANSPARENT)}};

#undef FN

#define FN(SIZE, OPACITY, TEXTURE) \
    &Renderer::DrawRectangleTextured<Rectangle::Size::SIZE, Rectangle::Opacity::OPACITY, Rectangle::Texture::TEXTURE>

    static constexpr DrawRectangleFunctions texture_functions[4][2][2] = {
        {{FN(ONE, OPAQUE, RAW),                 FN(ONE, OPAQUE, BLENDING)},
         {FN(ONE, SEMI_TRANSPARENT, RAW),       FN(ONE, SEMI_TRANSPARENT, BLENDING)}},
        {{FN(EIGHT, OPAQUE, RAW),               FN(EIGHT, OPAQUE, BLENDING)},
         {FN(EIGHT, SEMI_TRANSPARENT, RAW),     FN(EIGHT, SEMI_TRANSPARENT, BLENDING)}},
        {{FN(SIXTEEN, OPAQUE, RAW),             FN(SIXTEEN, OPAQUE, BLENDING)},
         {FN(SIXTEEN, SEMI_TRANSPARENT, RAW),   FN(SIXTEEN, SEMI_TRANSPARENT, BLENDING)}},
        {{FN(VARIABLE, OPAQUE, RAW),            FN(VARIABLE, OPAQUE, BLENDING)},
         {FN(VARIABLE, SEMI_TRANSPARENT, RAW),  FN(VARIABLE, SEMI_TRANSPARENT, BLENDING)}}};

#undef FN

    // call the appropriate draw function
    if (mode == Rectangle::DrawMode::MONO) {
        (this->*mono_functions[u32(size)][u32(opacity)])();
    } else {
        (this->*texture_functions[u32(size)][u32(opacity)][u32(texture)])();
    }
}

template <Rectangle::Size size>
static constexpr std::tuple<u16, u16> GetSize(Rectangle& rect) {
#define MAKE(x, y) std::make_tuple((u16) x, (u16) y)

    switch (size) {
        case Rectangle::Size::ONE: return MAKE(1, 1);
        case Rectangle::Size::EIGHT: return MAKE(8, 8);
        case Rectangle::Size::SIXTEEN: return MAKE(16, 16);
        case Rectangle::Size::VARIABLE: return MAKE(rect.size_x, rect.size_y);
    }

#undef MAKE
}

template <Rectangle::Size size, Rectangle::Opacity opacity>
void Renderer::DrawRectangleMono() {
    auto& rect = gpu->rectangle;

    auto [size_x, size_y] = GetSize<size>(rect);
    DebugAssert(size_x < 1024);
    DebugAssert(size_y < 512);

    const u16 end_x = rect.start_x + size_x;
    const u16 end_y = rect.start_y + size_y;

    for (u16 y = rect.start_y; y < end_y; y++) {
        for (u16 x = rect.start_x; x < end_x; x++) {
            gpu->vram[x + GPU::VRAM_WIDTH * y] = rect.c.To5551();
        }
    }
}

template <Rectangle::Size size, Rectangle::Opacity opacity, Rectangle::Texture texture>
void Renderer::DrawRectangleTextured() {
    auto& rect = gpu->rectangle;

    auto [size_x, size_y] = GetSize<size>(rect);
    DebugAssert(size_x < 1024);
    DebugAssert(size_y < 512);

    const u16 end_x = rect.start_x + size_x;
    const u16 end_y = rect.start_y + size_y;

    for (u16 y = rect.start_y; y < end_y; y++) {
        for (u16 x = rect.start_x; x < end_x; x++) {
            gpu->vram[x + GPU::VRAM_WIDTH * y] = 0xFF;
        }
    }
}

template void Renderer::DrawTriangle<DrawMode::MONO>(Vertex *v0, Vertex *v1, Vertex *v2);
template void Renderer::DrawTriangle<DrawMode::SHADED>(Vertex *v0, Vertex *v1, Vertex *v2);
template void Renderer::DrawTriangle<DrawMode::TEXTURE>(Vertex *v0, Vertex *v1, Vertex *v2);
