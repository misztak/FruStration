#pragma once

#include "vector"

#include "GL/gl3w.h"

#include "renderer.h"

struct VAO {
    VAO();
    void Reset();

    void Bind();
private:
    GLuint handle = 0;

    static GLuint currently_bound;
};
