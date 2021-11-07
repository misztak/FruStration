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
        return (0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
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

struct Rectangle {
    u16 start_x, start_y;
    u16 size_x, size_y;
    Color c;
    u8 tex_x, tex_y;
    ALWAYS_INLINE void SetStart(u32 value) {
        start_x = value, start_y = value >> 16;
    }
    ALWAYS_INLINE void SetSize(u32 value) {
        size_x = value, size_y = value >> 16;
    }
    ALWAYS_INLINE void SetTextPoint(u32 value) {
        tex_x = value, tex_y = value >> 8;
    }
};

enum class RectSize : u32 {
    ONE, EIGHT, SIXTEEN, VARIABLE
};

class Renderer {
public:
    Renderer(GPU* gpu);

    void Draw(u32 cmd);

    u16 palette = 0;
private:
    // draw flags
    static constexpr u32 NO_FLAGS = 0u;
    static constexpr u32 SHADED   = (1u << 0);
    static constexpr u32 TEXTURED = (1u << 1);
    static constexpr u32 OPAQUE   = (1u << 2);
    static constexpr u32 BLENDING = (1u << 3);
    // second triangle during 4-point polygon draw call
    static constexpr u32 SECOND_TRIANGLE = (1u << 4);

    template <u32 draw_flags>
    void DrawTriangle();

    template <u32 draw_flags>
    void Draw4PointPolygon();

    template <RectSize size, u32 draw_flags>
    void DrawRectangle();

    u16 GetTexel(u8 tex_x, u8 tex_y);

    GPU* gpu = nullptr;
};
