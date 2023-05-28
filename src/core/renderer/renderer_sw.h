#pragma once

#include <array>

#include "renderer/renderer.h"
#include "util/types.h"

class GPU;

// Software rasterizer (slow)
class Renderer_SW : public Renderer {
public:
    explicit Renderer_SW(GPU* gpu);

    void Draw(u32 cmd) override;

private:
    // draw flags
    static constexpr u32 MONO               = 1u << 0;
    static constexpr u32 SHADED             = 1u << 1;
    static constexpr u32 TEXTURED           = 1u << 2;
    static constexpr u32 OPAQUE             = 1u << 3;
    static constexpr u32 SEMI_TRANSPARENT   = 1u << 4;
    static constexpr u32 TEXTURE_BLENDING   = 1u << 5;
    static constexpr u32 RAW_TEXTURE        = 1u << 6;
    // second triangle during 4-point polygon draw call
    static constexpr u32 SECOND_TRIANGLE    = 1u << 7;

    static constexpr u16 TEXEL_MASK = 0x7FFF;

    template<u32 draw_flags>
    void DrawTriangle();

    template<u32 draw_flags>
    void Draw4PointPolygon();

    template<RectSize size, u32 draw_flags>
    void DrawRectangle();

    template<u32 draw_flags>
    void DrawLine();

    u16 GetTexel(u8 tex_x, u8 tex_y);

    GPU* gpu = nullptr;
};
