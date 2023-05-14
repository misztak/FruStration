#pragma once

#include <GL/gl3w.h>
#include <SDL.h>

#include <chrono>

#include "util/types.h"

class Emulator;

class Display {
public:
    Display();
    bool Init(Emulator* system, SDL_Window* window, SDL_GLContext context, const char* glsl_version);
    void Draw();
    void Render();
    void Update();
    void Throttle(u32 fps);

    static constexpr u32 WIDTH = 1200;
    static constexpr u32 HEIGHT = 800;

    bool vsync_enabled = true;
private:
    bool show_demo_window = false;
    bool show_stats_window = true;

    GLuint vram_tex_handler = 0;

    GLuint output_tex_handler = 0;

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;

    Emulator* emu = nullptr;

    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
};