#pragma once

#include <array>

#include "types.h"

class GPU;

struct Color {
    Color() {};
    Color(u8 r, u8 g, u8 b) : r(r), g(g), b(b) {}
    Color(u32 val) : r(val & 0xFF), g((val >> 8) & 0xFF), b((val >> 16) & 0xFF) {}
    u8 r = 0, g = 0, b = 0;
    ALWAYS_INLINE u16 To5551() const { return (0x8000 | ((b & 0x1F) << 10) | ((g & 0x1F) << 5) | (r & 0x1F)); }
};

struct Vertex {
    Vertex(u16 x, u16 y, Color c) : x(x), y(y), c(c) {}
    Vertex(u32 val, Color c) : x(val & 0xFFFF), y(val >> 16), c(c) {}
    Vertex(u32 val, Color c, u32 tex_coord)
        : x(val & 0xFFFF), y(val >> 16), c(c), tex_x(tex_coord), tex_y(tex_coord >> 8) {}
    u16 x, y;
    Color c = 0;
    u8 tex_x = 0, tex_y = 0;
};

class Renderer {
public:
    Renderer();
    void Init(GPU* gpu);
    void DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2);

    enum class DrawMode : u32 {
        CLEAR,
        MONO,
        SHADED,
        TEXTURE,
    };
    DrawMode draw_mode = DrawMode::CLEAR;

    u16 palette = 0;

    std::array<u8, 4> pixel_modes = { 4, 8, 16, 0 };
private:
    u16 GetTexel4Bit(u16 tex_x, u16 clut_index, u16 clut_x, u16 clut_y);
    u16 GetTexel8Bit(u16 tex_x, u16 tex_y, u16 clut_x, u16 clut_y, u16 page_x, u16 page_y);
    u16 GetTexel16Bit(u16 tex_x, u16 tex_y, u16 clut_x, u16 clut_y, u16 page_x, u16 page_y);

    GPU* gpu = nullptr;
};