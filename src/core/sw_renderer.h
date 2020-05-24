#pragma once

#include "types.h"

class GPU;

struct Color {
    Color(u8 r, u8 g, u8 b) : r(r), g(g), b(b) {}
    Color(u32 val) : r(val & 0xFF), g((val >> 8) & 0xFF), b((val >> 16) & 0xFF) {}
    u8 r, g, b;
    ALWAYS_INLINE u16 To5551() const { return (0x8000 | ((b & 0x1F) << 10) | ((g & 0x1F) << 5) | (r & 0x1F)); }
};

struct Vertex {
    Vertex(u16 x, u16 y, Color c) : x(x), y(y), c(c) {}
    Vertex(u32 val, Color c) : x(val & 0xFFFF), y(val >> 16), c(c) {}
    u16 x, y;
    Color c;
};

class Renderer {
public:
    Renderer();
    void Init(GPU* gpu);
    void DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2);

    static constexpr u32 DRAW_FLAG_CLEAR    = 0;
    static constexpr u32 DRAW_FLAG_MONO     = 1 << 0;
    static constexpr u32 DRAW_FLAG_SHADED   = 1 << 1;
    static constexpr u32 DRAW_FLAG_TEXTURE  = 1 << 2;
    u32 draw_flags = DRAW_FLAG_CLEAR;
private:

    GPU* gpu = nullptr;
};