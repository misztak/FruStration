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

    enum class DrawMode : u32 {
        MONO, TEXTURED
    };
    enum class Size : u32 {
        ONE, EIGHT, SIXTEEN, VARIABLE
    };
    enum class Opacity : u32 {
        OPAQUE, SEMI_TRANSPARENT
    };
    enum class Texture : u32 {
        RAW, BLENDING, NONE
    };
};

enum class DrawMode : u32 {
    MONO,
    SHADED,
    TEXTURE,
};

class Renderer {
public:
    Renderer();
    void Init(GPU* gpu);

    template <DrawMode mode>
    void DrawTriangle(Vertex* v0, Vertex* v1, Vertex* v2);
    template <DrawMode mode>
    void DrawTriangleAVX2(Vertex* v0, Vertex* v1, Vertex* v2, u16 minX, u16 maxX, u16 minY, u16 maxY);

    void DrawRectangleWith(Rectangle::DrawMode mode, Rectangle::Size size, Rectangle::Opacity opacity, Rectangle::Texture texture);

    template <Rectangle::Size size, Rectangle::Opacity opacity>
    void DrawRectangleMono();

    template <Rectangle::Size size, Rectangle::Opacity opacity, Rectangle::Texture texture>
    void DrawRectangleTextured();

    u16 palette = 0;
private:
    GPU* gpu = nullptr;
};