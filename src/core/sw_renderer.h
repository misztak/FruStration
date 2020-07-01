#pragma once

#include <array>

#include "types.h"

class GPU;

struct Color {
    u8 r, g, b;
    ALWAYS_INLINE void SetColor(u32 value) {
        r = u8(value), g = u8(value >> 8), b = u8(value >> 16);
    }
    ALWAYS_INLINE u16 To5551() const {
        return (0x8000 | ((b & 0x1F) << 10) | ((g & 0x1F) << 5) | (r & 0x1F));
    }
};

struct Vertex {
    s16 x, y;
    Color c;
    u8 tex_x, tex_y;
    ALWAYS_INLINE void SetPoint(u32 value) {
        x = s16(value), y = s16(value >> 16);
    }
    ALWAYS_INLINE void SetTextPoint(u32 value) {
        tex_x = u8(value), tex_y = u8(value >> 8);
    }
    ALWAYS_INLINE void Reset() {
        x = 0, y = 0, c.r = 0, c.g = 0, c.b = 0, tex_x = 0, tex_y = 0;
    }
};

class Renderer {
public:
    Renderer();
    void Init(GPU* gpu);
    void DrawTriangle(Vertex* v0, Vertex* v1, Vertex* v2);

    enum class DrawMode : u32 {
        CLEAR,
        MONO,
        SHADED,
        TEXTURE,
    };
    DrawMode draw_mode = DrawMode::CLEAR;

    u16 palette = 0;
private:
    GPU* gpu = nullptr;
};