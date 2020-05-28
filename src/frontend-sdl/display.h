#pragma once

#include <GL/gl3w.h>
#include <SDL.h>

#include <chrono>

#include "types.h"

class System;

class Display {
public:
    Display();
    bool Init(System* system, SDL_Window* window, SDL_GLContext context, const char* glsl_version);
    void Draw(bool* done, bool vsync);
    void Render();
    void Throttle(u32 fps);

    static constexpr u32 WIDTH = 1200;
    static constexpr u32 HEIGHT = 800;

private:
    bool show_demo_window = false;
    bool show_stats_window = true;

    GLuint vram_tex_handler = 0;

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;

    System* emu = nullptr;

    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
};