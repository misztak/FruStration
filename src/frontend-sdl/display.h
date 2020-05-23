#pragma once

#include <GL/gl3w.h>
#include <SDL.h>

#include <chrono>

#include "types.h"

class Display {
public:
    Display();
    bool Init(SDL_Window* window, SDL_GLContext context, const char* glsl_version);
    void Draw(bool* done, bool vsync);
    void Render();
    void Throttle(u32 fps);

    static constexpr u32 WIDTH = 1024 * 2;
    static constexpr u32 HEIGHT = 512 * 2;

private:
    bool show_demo_window = false;
    bool show_stats_window = true;

    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
};