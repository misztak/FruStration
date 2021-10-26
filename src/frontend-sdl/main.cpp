#include <GL/gl3w.h>
#include <SDL.h>
#include <iostream>

#include "display.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "system.h"
#include "macros.h"
#include "scheduler.h"

LOG_CHANNEL(MAIN);

constexpr bool RUN_HEADLESS = false;

int RunCore() {
    const std::string bios_path = "SCPH1001.BIN";

    System system;
    system.Init();
    if (!system.LoadBIOS(bios_path)) return 1;

    Scheduler::RecalculateNextEvent();

    while (true) system.Tick();
    return 0;
}

int main(int, char**) {
    // init nanolog
    nanolog::initialize(nanolog::GuaranteedLogger());
    nanolog::set_log_level(nanolog::LogLevel::DBG);

    if constexpr (RUN_HEADLESS) {
        return RunCore();
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_CRIT << "Error: " << SDL_GetError();
        return 1;
    }
    // some linux WMs/Compositors will render artifacts and have other issues if this is not enabled
    // SDL_SetHintWithPriority(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0", SDL_HINT_OVERRIDE);

#if __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("FruStration", 3840, 0,
                                          Display::WIDTH, Display::HEIGHT, window_flags);
    if (!window) {
        LOG_CRIT << "Failed to create SDL_Window";
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    bool err = gl3wInit() != 0;
    if (err) {
        LOG_CRIT << "Failed to initialize OpenGL loader";
        return 1;
    }

    System system;
    system.Init();
    if (!system.LoadBIOS("SCPH1001.BIN")) return 1;
    //if (!system.LoadBIOS("SCPH7002.BIN")) return 1;

    system.StartGDBServer();

    Display display;
    if (!display.Init(&system, window, gl_context, glsl_version)) {
        LOG_CRIT << "Failed to initialize imgui display";
        return 1;
    };

    Scheduler::RecalculateNextEvent();

    while (!system.done) {
        if (!system.IsHalted()) {

            while (!system.DrawNextFrame()) {
                system.Tick();
                // cpu reached a breakpoint
                if (unlikely(system.IsHalted())) break;
            }

            // check if ready to draw next frame again because the cpu could have hit a breakpoint
            // before reaching the next vblank
            if (system.DrawNextFrame()) {
                system.ResetDrawFrame();
                display.Update();
            }
        } else {
            display.Update();
            // if the GDB server is disabled this will do nothing
            system.HandleGDBClientRequest();
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}