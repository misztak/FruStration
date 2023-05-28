#pragma once

#include "GL/gl3w.h"

#include "renderer.h"
#include "opengl_helper.h"

class GPU;

class Renderer_OpenGL : public Renderer {
public:
    explicit Renderer_OpenGL(GPU* gpu);
    Renderer_OpenGL() = delete;

    void Reset();

    void Draw(u32 cmd) override;

private:
    VAO displayVAO;
    VAO vramVAO;

    GPU* gpu = nullptr;
};