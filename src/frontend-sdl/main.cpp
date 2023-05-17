#include <string_view>

#include <GL/gl3w.h>
#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "common/asserts.h"
#include "common/config.h"
#include "common/log.h"
#include "display.h"
#include "emulator.h"

LOG_CHANNEL(MAIN);

void PrintUsageAndExit(int exit_code);

int main(int argc, char* argv[]) {

    // parse command line arguments

    if (argc < 1 || argc > 7) PrintUsageAndExit(1);

    std::string arg_bios_path, arg_bin_path, arg_psexe_path;

    for (int i = 1; i < argc; i++) {
        std::string_view arg(argv[i]);

        if (arg == "-h" || arg == "--help") PrintUsageAndExit(0);

        if (arg == "-d" || arg == "--debug") {
            // TODO
            continue;
        }

        if (arg == "-B" || arg == "--bios") {
            if (i + 1 >= argc) PrintUsageAndExit(1);
            arg_bios_path = std::string(argv[i++ + 1]);
            continue;
        }
        if (arg == "-b" || arg == "--bin") {
            if (i + 1 >= argc) PrintUsageAndExit(1);
            arg_bin_path = std::string(argv[i++ + 1]);
            continue;
        }
        if (arg == "-e" || arg == "--psexe") {
            if (i + 1 >= argc) PrintUsageAndExit(1);
            arg_psexe_path = std::string(argv[i++ + 1]);
            continue;
        }

        printf("Unknown argument '%s'\n", arg.data());
        PrintUsageAndExit(1);
    }

    // initialize logger
#ifndef NDEBUG
    Log::Init(spdlog::level::trace);
#else
    Log::Init(spdlog::level::info);
#endif

    Config::LoadConfig();

    Config::ps_bin_file_path = arg_bin_path;
    Config::psexe_file_path = arg_psexe_path;

    // use BIOS path from args if specified (overwrites config file value)
    if (!arg_bios_path.empty()) Config::bios_path.Set(arg_bios_path);

    // psexe overwrites
    //Config::psexe_file_path = "../test/exe/hello_world.exe";
    //Config::psexe_file_path = "../test/exe/timer.exe";
    //Config::psexe_file_path = "../test/exe/n00bdemo.exe";
    //Config::psexe_file_path = "../test/exe/amidog/psxtest_cpu.exe";
    //Config::psexe_file_path = "../test/exe/amidog/psxtest_cpx.exe";
    //Config::psexe_file_path = "../test/exe/avocado/HelloWorld16BPP.exe";
    //Config::psexe_file_path = "../test/exe/avocado/HelloWorld24BPP.exe";
    //Config::psexe_file_path = "../test/exe/avocado/ImageLoad.exe";

    Emulator emulator;

    if (!emulator.LoadBIOS()) return 1;

    if (Config::gdb_server_enabled.Get()) emulator.StartGDBServer();

    emulator.SetPaused(true);

    // init display

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LogCrit("Error: {}", SDL_GetError());
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
    SDL_Window* window = SDL_CreateWindow("FruStration", SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0),
                                          Display::WIDTH, Display::HEIGHT, window_flags);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    if (!window) {
        LogCrit("Failed to create SDL_Window");
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    bool err = gl3wInit() != 0;
    if (err) {
        LogCrit("Failed to initialize OpenGL loader");
        return 1;
    }

    Display display;
    if (!display.Init(&emulator, window, gl_context, glsl_version)) {
        LogCrit("Failed to initialize imgui display");
        return 1;
    }

    // vsync
    SDL_GL_SetSwapInterval(static_cast<s32>(display.vsync_enabled));

    while (!emulator.done) {
        if (!emulator.IsPaused()) {

            while (!emulator.DrawNextFrame()) {
                emulator.Tick();
                // cpu reached a breakpoint
                if (unlikely(emulator.IsPaused())) break;
            }

            // check if ready to draw next frame again because the cpu could have hit a breakpoint
            // before reaching the next vblank
            if (emulator.DrawNextFrame()) {
                emulator.ResetDrawFrame();
                display.Update();
            }

            //display.Throttle(60);

        } else {
            display.Update();
            // if the GDB server is disabled this will do nothing
            emulator.HandleGDBClientRequest();
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    Log::Shutdown();

    return 0;
}

void PrintUsageAndExit(int exit_code) {
    printf("Usage: frustration [OPTIONS]\n\n");
    printf("Options:\n");
    printf("    -h, --help          Display this message\n");
    printf("    -d, --debug         Start emulator with debug tools enabled\n");
    printf("    -B, --bios FILE     Set the BIOS source (overwrites config file)\n");
    printf("    -b, --bin FILE      Execute the PSX binary \n");
    printf("    -e, --psexe FILE    Inject and run the PSEXE file\n\n");

    std::exit(exit_code);
}
