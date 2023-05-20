#pragma once

#include "util/types.h"

// TODO: improve the design of these structs

struct Color {
    u8 r, g, b;
    ALWAYS_INLINE void SetColor(u32 value) { r = u8(value), g = u8(value >> 8), b = u8(value >> 16); }
    ALWAYS_INLINE u16 To5551() const { return (0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)); }
};

struct Vertex {
    s16 x, y;
    Color c;
    u8 tex_x, tex_y;
    ALWAYS_INLINE void SetPoint(u32 value) { x = s16(value), y = s16(value >> 16); }
    ALWAYS_INLINE void SetTextPoint(u32 value) { tex_x = u8(value), tex_y = u8(value >> 8); }
    ALWAYS_INLINE void Reset() { x = 0, y = 0, c.r = 0, c.g = 0, c.b = 0, tex_x = 0, tex_y = 0; }
};

struct Rectangle {
    u16 start_x, start_y;
    u16 size_x, size_y;
    Color c;
    u8 tex_x, tex_y;
    ALWAYS_INLINE void SetStart(u32 value) { start_x = value, start_y = value >> 16; }
    ALWAYS_INLINE void SetSize(u32 value) { size_x = value, size_y = value >> 16; }
    ALWAYS_INLINE void SetTextPoint(u32 value) { tex_x = value, tex_y = value >> 8; }
};

enum class RectSize : u32 { ONE, EIGHT, SIXTEEN, VARIABLE };

// Base class for a render backend
struct Renderer {

    virtual ~Renderer() = default;

    virtual void Draw(u32 cmd) = 0;
};
