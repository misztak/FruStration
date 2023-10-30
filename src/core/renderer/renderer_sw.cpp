#include "renderer_sw.h"

#include <algorithm>
#include <tuple>

#include "common/asserts.h"
#include "common/log.h"
#include "gpu.h"

LOG_CHANNEL(Renderer);

Renderer_SW::Renderer_SW(GPU* gpu) : gpu(gpu) {
    LogInfo("Graphics backend: Software Renderer (SW)");
}

static constexpr s32 EdgeFunction(Vertex* v0, Vertex* v1, Vertex* v2) {
    return (s32)((v1->x - v0->x) * (v2->y - v0->y) - (v1->y - v0->y) * (v2->x - v0->x));
}

static constexpr s32 EdgeFunction(Vertex* v0, Vertex* v1, s16 px, s16 py) {
    return (s32)((v1->x - v0->x) * (py - v0->y) - (v1->y - v0->y) * (px - v0->x));
}

static constexpr Color BlendTexture(Color& tx_color, Color& blend_color) {
    Color blend = {};
    blend.r = u8(std::min(((u32(tx_color.r) * u32(blend_color.r)) / 128u), 255u));
    blend.g = u8(std::min(((u32(tx_color.g) * u32(blend_color.g)) / 128u), 255u));
    blend.b = u8(std::min(((u32(tx_color.b) * u32(blend_color.b)) / 128u), 255u));
    return blend;
}

static constexpr Color CalcSemiTransparency(u32 mode, Color& back, Color& front) {
    Color c = {};

    auto ApplyToChannels = [&](s32 back_div, s32 front_div, s32 sign){
        c.r = u8(std::clamp((s32(back.r) / back_div) + (s32(front.r) / front_div) * sign, 0, 255));
        c.g = u8(std::clamp((s32(back.g) / back_div) + (s32(front.g) / front_div) * sign, 0, 255));
        c.b = u8(std::clamp((s32(back.b) / back_div) + (s32(front.b) / front_div) * sign, 0, 255));
    };

    switch (mode) {
        case 0:
            ApplyToChannels(2, 2, +1);
            break;
        case 1:
            ApplyToChannels(1, 1, +1);
            break;
        case 2:
            ApplyToChannels(1, 1, -1);
            break;
        case 3:
            ApplyToChannels(1, 4, +1);
            break;
        default: break;
    }

    return c;
}

#define DRAW_FLAGS_SET(flags) ((draw_flags & (flags)) != 0)

template<u32 draw_flags>
void Renderer_SW::DrawTriangle() {
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
    minX = std::clamp<s32>(minX + gpu->drawing_x_offset, gpu->drawing_area_left, gpu->drawing_area_right);
    maxX = std::clamp<s32>(maxX + gpu->drawing_x_offset, gpu->drawing_area_left, gpu->drawing_area_right);
    minY = std::clamp<s32>(minY + gpu->drawing_y_offset, gpu->drawing_area_top, gpu->drawing_area_bottom);
    maxY = std::clamp<s32>(maxY + gpu->drawing_y_offset, gpu->drawing_area_top, gpu->drawing_area_bottom);

    // prepare per-pixel increments
    const s32 A01 = v0->y - v1->y, B01 = v1->x - v0->x;
    const s32 A12 = v1->y - v2->y, B12 = v2->x - v1->x;
    const s32 A20 = v2->y - v0->y, B20 = v0->x - v2->x;

    // area of the parallelogram
    // weights for interpolation will have to be normalized by the area
    const s32 area = EdgeFunction(v0, v1, v2);
    // colinear, nothing to draw
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
                Color px_color = {};

                if constexpr (DRAW_FLAGS_SET(MONO)) {
                    // all vertices store the same color value
                    px_color = v0->c;
                }

                bool transparency_enabled = false;

                if constexpr (DRAW_FLAGS_SET(TEXTURED)) {
                    u8 tex_x = std::clamp((v0->tex_x * w12 + v1->tex_x * w20 + v2->tex_x * w01) / area, 0, 255);
                    u8 tex_y = std::clamp((v0->tex_y * w12 + v1->tex_y * w20 + v2->tex_y * w01) / area, 0, 255);

                    u16 texel = GetTexel(tex_x, tex_y);
                    transparency_enabled = gpu->status.tex_page_colors == 2 || (texel & 0x8000) != 0;
                    if (texel & TEXEL_MASK) {
                        px_color = Color::FromU16(texel);
                        if constexpr (DRAW_FLAGS_SET(TEXTURE_BLENDING) && !DRAW_FLAGS_SET(SHADED)) {
                            px_color = BlendTexture(px_color, v0->c);
                        }
                    } else {
                        // leave early
                        w01 += A01; w12 += A12; w20 += A20;
                        continue;
                    }
                }

                if constexpr (DRAW_FLAGS_SET(SHADED)) {
                    Color color;

                    color.r = (s32(v0->c.r) * w12 + s32(v1->c.r) * w20 + s32(v2->c.r) * w01) / area;
                    color.g = (s32(v0->c.g) * w12 + s32(v1->c.g) * w20 + s32(v2->c.g) * w01) / area;
                    color.b = (s32(v0->c.b) * w12 + s32(v1->c.b) * w20 + s32(v2->c.b) * w01) / area;

                    if constexpr (DRAW_FLAGS_SET(TEXTURE_BLENDING)) {
                        px_color = BlendTexture(px_color, color);
                    } else {
                        px_color = color;
                    }
                }

                if constexpr (DRAW_FLAGS_SET(SEMI_TRANSPARENT)) {
                    if (transparency_enabled) {
                        Color old = Color::FromU16(gpu->vram[px + GPU::VRAM_WIDTH * py]);
                        Color mix = CalcSemiTransparency(gpu->status.semi_transparency, old, px_color);
                        gpu->vram[px + GPU::VRAM_WIDTH * py] = mix.To5551();
                    } else {
                        gpu->vram[px + GPU::VRAM_WIDTH * py] = px_color.To5551();
                    }
                } else {
                    gpu->vram[px + GPU::VRAM_WIDTH * py] = px_color.To5551();
                }
                //gpu->vram[px + GPU::VRAM_WIDTH * py] = px_color.To5551();
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

template<u32 draw_flags>
void Renderer_SW::Draw4PointPolygon() {
    // build 4-point polygon using two calls to DrawTriangle
    DrawTriangle<draw_flags>();
    DrawTriangle<draw_flags | SECOND_TRIANGLE>();
}

template<RectSize size>
static constexpr std::tuple<u16, u16> GetSize(Rectangle& rect) {
#define MAKE(x, y) std::make_tuple(u16(x), u16(y))

    switch (size) {
        case RectSize::ONE: return MAKE(1, 1);
        case RectSize::EIGHT: return MAKE(8, 8);
        case RectSize::SIXTEEN: return MAKE(16, 16);
        case RectSize::VARIABLE: return MAKE(rect.size_x, rect.size_y);
    }

#undef MAKE
}

template<RectSize size, u32 draw_flags>
void Renderer_SW::DrawRectangle() {
    auto& rect = gpu->rectangle;

    auto [size_x, size_y] = GetSize<size>(rect);

    const s32 start_with_offset_x = (s32)rect.start_x + (s32)gpu->drawing_x_offset;
    const s32 start_with_offset_y = (s32)rect.start_y + (s32)gpu->drawing_y_offset;

    const s32 start_x = std::clamp<s32>(start_with_offset_x, gpu->drawing_area_left, gpu->drawing_area_right);
    const s32 start_y = std::clamp<s32>(start_with_offset_y, gpu->drawing_area_top, gpu->drawing_area_bottom);
    const s32 end_x = std::clamp<s32>(start_with_offset_x + size_x, gpu->drawing_area_left, gpu->drawing_area_right);
    const s32 end_y = std::clamp<s32>(start_with_offset_y + size_y, gpu->drawing_area_top, gpu->drawing_area_bottom);

    //LogTrace("Rect<{},{}> from (x={},y={}) to (x={},y={})", DRAW_FLAGS_SET(TEXTURED) ? "Textured" : "Mono",
    //         DRAW_FLAGS_SET(OPAQUE) ? "Opaque" : "SemiTransparent", start_x, start_y, end_x, end_y);

    for (s32 y = start_y; y < end_y; y++) {
        for (s32 x = start_x; x < end_x; x++) {
            Color px_color = {};

            bool transparency_enabled = true;

            if constexpr (DRAW_FLAGS_SET(TEXTURED)) {
                const s32 tex_y_inc_dir = gpu->tex_rectangle_yflip ? -1 : +1;
                const s32 tex_x_inc_dir = gpu->tex_rectangle_xflip ? -1 : +1;

                u16 texel = GetTexel(rect.tex_x + (x - start_x) * tex_x_inc_dir, rect.tex_y + (y - start_y) * tex_y_inc_dir);

                if (gpu->status.tex_page_colors != 2 && (texel & 0x8000) == 0) transparency_enabled = false;

                if (texel & TEXEL_MASK) {
                    px_color = Color::FromU16(texel);
                    if constexpr (DRAW_FLAGS_SET(TEXTURE_BLENDING)) {
                        px_color = BlendTexture(px_color, rect.c);
                    }
                } else {
                    continue;
                }
            } else {
                // mono
                px_color = rect.c;
            }

            if constexpr (DRAW_FLAGS_SET(SEMI_TRANSPARENT)) {
                if (transparency_enabled) {
                    Color old = Color::FromU16(gpu->vram[x + GPU::VRAM_WIDTH * y]);
                    Color mix = CalcSemiTransparency(gpu->status.semi_transparency, old, px_color);
                    gpu->vram[x + GPU::VRAM_WIDTH * y] = mix.To5551();
                } else {
                    gpu->vram[x + GPU::VRAM_WIDTH * y] = px_color.To5551();
                }
            } else {
                gpu->vram[x + GPU::VRAM_WIDTH * y] = px_color.To5551();
            }
        }
    }

}

template<u32 draw_flags>
void Renderer_SW::DrawLine() {
    if (gpu->line_buffer.size() < 2) {
        LogWarn("Trying to draw a line without at least 2 points");
        return;
    }

    for (usize i = 0; i < gpu->line_buffer.size() - 1; i++) {
        Vertex& p0 = gpu->line_buffer[i];
        Vertex& p1 = gpu->line_buffer[i+1];

        s32 x0 = std::clamp<s32>(p0.x + gpu->drawing_x_offset, gpu->drawing_area_left, gpu->drawing_area_right);
        s32 y0 = std::clamp<s32>(p0.y + gpu->drawing_y_offset, gpu->drawing_area_top, gpu->drawing_area_bottom);

        s32 x1 = std::clamp<s32>(p1.x + gpu->drawing_x_offset, gpu->drawing_area_left, gpu->drawing_area_right);
        s32 y1 = std::clamp<s32>(p1.y + gpu->drawing_y_offset, gpu->drawing_area_top, gpu->drawing_area_bottom);

        s32 dx = std::abs(x1 - x0);
        s32 sx = x0 < x1 ? 1 : -1;
        s32 dy = -std::abs(y1 - y0);
        s32 sy = y0 < y1 ? 1 : -1;
        s32 error = dx + dy;

        while (true) {
            if constexpr (DRAW_FLAGS_SET(SHADED)) {
                // TODO
                gpu->vram[x0 + GPU::VRAM_WIDTH * y0] = 0x7FFF;
            } else {
                gpu->vram[x0 + GPU::VRAM_WIDTH * y0] = p0.c.To5551();
            }

            // TODO: semi transparency

            if (x0 == x1 && y0 == y1) break;
            s32 e2 = 2 * error;
            if (e2 >= dy) {
                if (x0 == x1) break;
                error = error + dy;
                x0 = x0 + sx;
            }
            if (e2 <= dx) {
                if (y0 == y1) break;
                error = error + dx;
                y0 = y0 + sy;
            }
        }
    }
}

#undef DRAW_FLAGS_SET

u16 Renderer_SW::GetTexel(u8 tex_x, u8 tex_y) {
    tex_x = (tex_x & ~(gpu->tex_window_x_mask * 8)) | ((gpu->tex_window_x_offset & gpu->tex_window_x_mask) * 8);
    tex_y = (tex_y & ~(gpu->tex_window_y_mask * 8)) | ((gpu->tex_window_y_offset & gpu->tex_window_y_mask) * 8);

    u16 base_x = gpu->status.tex_page_x_base * 64;
    u16 base_y = gpu->status.tex_page_y_base * 256;

    u8 tex_mode = gpu->status.tex_page_colors;
    u16 texel;

    switch (tex_mode) {
        case 0:
        {
            u32 tx = std::min<u32>(base_x + (tex_x / 4), 1023u);
            u32 ty = std::min<u32>(base_y + tex_y, 511u);

            u16 palette_value = gpu->vram[tx + GPU::VRAM_WIDTH * ty];
            u16 palette_index = (palette_value >> ((tex_x % 4) * 4)) & 0xFu;

            u16 texel_x = std::min<u32>((gpu->clut & 0x3F) * 16 + palette_index, 1023u);
            u16 texel_y = (gpu->clut >> 6) & 0x1FF;

            texel = gpu->vram[texel_x + GPU::VRAM_WIDTH * texel_y];
            break;
        }
        case 1:
        {
            u32 tx = std::min<u32>(base_x + (tex_x / 2), 1023u);
            u32 ty = std::min<u32>(base_y + tex_y, 511u);

            u16 palette_value = gpu->vram[tx + GPU::VRAM_WIDTH * ty];
            u16 palette_index = (palette_value >> ((tex_x % 2) * 8)) & 0xFFu;

            u16 texel_x = std::min<u32>((gpu->clut & 0x3F) * 16 + palette_index, 1023u);
            u16 texel_y = (gpu->clut >> 6) & 0x1FF;

            texel = gpu->vram[texel_x + GPU::VRAM_WIDTH * texel_y];
            break;
        }
        case 2:
        {
            u32 texel_x = (base_x + tex_x) & 1023u;
            u32 texel_y = (base_y + tex_y) & 511u;

            texel = gpu->vram[texel_x + GPU::VRAM_WIDTH * texel_y];
            break;
        }
        default:
            texel = 0xFF;
            LogWarn("Invalid texture color mode 3");
    }

    return texel;
}

void Renderer_SW::Draw(u32 cmd) {
    // clang-format off

    switch ((cmd >> 24)) {
        case 0x20: DrawTriangle<MONO | OPAQUE>(); break;
        case 0x22: DrawTriangle<MONO | SEMI_TRANSPARENT>(); break;
        case 0x24: DrawTriangle<TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x25: DrawTriangle<TEXTURED | OPAQUE | RAW_TEXTURE>(); break;
        case 0x26: DrawTriangle<TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;
        case 0x27: DrawTriangle<TEXTURED | SEMI_TRANSPARENT | RAW_TEXTURE>(); break;
        case 0x30: DrawTriangle<SHADED | OPAQUE>(); break;
        case 0x32: DrawTriangle<SHADED | SEMI_TRANSPARENT>(); break;
        case 0x34: DrawTriangle<SHADED | TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x36: DrawTriangle<SHADED | TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;

        case 0x28: Draw4PointPolygon<MONO | OPAQUE>(); break;
        case 0x2A: Draw4PointPolygon<MONO | SEMI_TRANSPARENT>(); break;
        case 0x2C: Draw4PointPolygon<TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x2D: Draw4PointPolygon<TEXTURED | OPAQUE | RAW_TEXTURE>(); break;
        case 0x2E: Draw4PointPolygon<TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;
        case 0x2F: Draw4PointPolygon<TEXTURED | SEMI_TRANSPARENT | RAW_TEXTURE>(); break;
        case 0x38: Draw4PointPolygon<SHADED | OPAQUE>(); break;
        case 0x3A: Draw4PointPolygon<SHADED | SEMI_TRANSPARENT>(); break;
        case 0x3C: Draw4PointPolygon<SHADED | TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x3E: Draw4PointPolygon<SHADED | TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;

        case 0x40: case 0x48: DrawLine<MONO | OPAQUE>(); break;
        case 0x42: case 0x4A: DrawLine<MONO | SEMI_TRANSPARENT>(); break;

        case 0x50: case 0x58: DrawLine<SHADED | OPAQUE>(); break;
        case 0x52: case 0x5A: DrawLine<SHADED | SEMI_TRANSPARENT>(); break;

        case 0x60: DrawRectangle<RectSize::VARIABLE, MONO | OPAQUE>(); break;
        case 0x62: DrawRectangle<RectSize::VARIABLE, MONO | SEMI_TRANSPARENT>(); break;
        case 0x68: DrawRectangle<RectSize::ONE, MONO | OPAQUE>(); break;
        case 0x6A: DrawRectangle<RectSize::ONE, MONO | SEMI_TRANSPARENT>(); break;
        case 0x70: DrawRectangle<RectSize::EIGHT, MONO | OPAQUE>(); break;
        case 0x72: DrawRectangle<RectSize::EIGHT, MONO | SEMI_TRANSPARENT>(); break;
        case 0x78: DrawRectangle<RectSize::SIXTEEN, MONO | OPAQUE>(); break;
        case 0x7A: DrawRectangle<RectSize::SIXTEEN, MONO | SEMI_TRANSPARENT>(); break;

        case 0x64: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x65: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE | RAW_TEXTURE>(); break;
        case 0x66: DrawRectangle<RectSize::VARIABLE, TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;
        case 0x67: DrawRectangle<RectSize::VARIABLE, TEXTURED | SEMI_TRANSPARENT | RAW_TEXTURE>(); break;

        // textured dots make no sense, implement if used by a game?
        //case 0x6C: DrawRectangle<RectSize::ONE, TEXTURED | OPAQUE | BLENDING>(); break;
        //case 0x6D: DrawRectangle<RectSize::ONE, TEXTURED | OPAQUE>(); break;
        //case 0x6E: DrawRectangle<RectSize::ONE, TEXTURED | BLENDING>(); break;
        //case 0x6F: DrawRectangle<RectSize::ONE, TEXTURED>(); break;

        case 0x74: DrawRectangle<RectSize::EIGHT, TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x75: DrawRectangle<RectSize::EIGHT, TEXTURED | OPAQUE | RAW_TEXTURE>(); break;
        case 0x76: DrawRectangle<RectSize::EIGHT, TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;
        case 0x77: DrawRectangle<RectSize::EIGHT, TEXTURED | SEMI_TRANSPARENT | RAW_TEXTURE>(); break;

        case 0x7C: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE | TEXTURE_BLENDING>(); break;
        case 0x7D: DrawRectangle<RectSize::VARIABLE, TEXTURED | OPAQUE | RAW_TEXTURE>(); break;
        case 0x7E: DrawRectangle<RectSize::VARIABLE, TEXTURED | SEMI_TRANSPARENT | TEXTURE_BLENDING>(); break;
        case 0x7F: DrawRectangle<RectSize::VARIABLE, TEXTURED | SEMI_TRANSPARENT | RAW_TEXTURE>(); break;

        default: Panic("Invalid draw command 0x{:08X}", cmd);
    }

    // clang-format on
}
