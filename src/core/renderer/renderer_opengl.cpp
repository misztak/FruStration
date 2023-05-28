#include "renderer_opengl.h"

#include "gpu.h"
#include "common/asserts.h"
#include "common/log.h"

LOG_CHANNEL(Renderer_OpenGL);

Renderer_OpenGL::Renderer_OpenGL(GPU *gpu) : gpu(gpu) {
    LogInfo("Initialized OpenGL renderer");
}

void Renderer_OpenGL::Draw(u32 cmd) {

}
