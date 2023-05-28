#include "opengl_helper.h"

#include "common/asserts.h"
#include "common/log.h"

LOG_CHANNEL(OpenGL);

GLuint VAO::currently_bound = 0;

VAO::VAO() {
    glGenVertexArrays(1, &handle);
}

void VAO::Bind() {
    Assert(handle != 0);

    if (handle != currently_bound) {
        glBindVertexArray(handle);
        currently_bound = handle;
    }
}

void VAO::Reset() {
    if (handle != 0) {
        if (currently_bound == handle) {
            glBindVertexArray(0);
            currently_bound = 0;
        }
        glDeleteVertexArrays(1, &handle);
        handle = 0;
    }
}
