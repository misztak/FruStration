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

void HandleInput(Emulator& emulator, Controller& controller, Display& display, SDL_Window* window);
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

        std::printf("Unknown argument '%s'\n", arg.data());
        PrintUsageAndExit(1);
    }

    // initialize logger
#ifndef NDEBUG
    Log::Init(spdlog::level::info);
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
    //Config::psexe_file_path = "/home/anon/Tools/psnoob/PSn00bSDK_examples/build/demos/n00bdemo/n00bdemo.exe";

    Emulator emulator;

    if (!emulator.LoadBIOS()) return 1;

    if (Config::gdb_server_enabled.Get()) emulator.StartGDBServer();

    // load the selected psexe file
    if (!Config::psexe_file_path.empty()) {
        bool success = emulator.LoadPsExe();
        if (!success) LogWarn("Failed to load PS-EXE file");
    }

    emulator.SetPaused(true);

    // configure controller (digital pad)
    Controller& controller = emulator.GetMainController();
    controller.SetKeyMap({
        {static_cast<u32>(SDL_SCANCODE_RETURN), Controller::Button::Start},
        {static_cast<u32>(SDL_SCANCODE_SPACE), Controller::Button::Select},
        {static_cast<u32>(SDL_SCANCODE_UP), Controller::Button::Up},
        {static_cast<u32>(SDL_SCANCODE_LEFT), Controller::Button::Left},
        {static_cast<u32>(SDL_SCANCODE_DOWN), Controller::Button::Down},
        {static_cast<u32>(SDL_SCANCODE_RIGHT), Controller::Button::Right},
        {static_cast<u32>(SDL_SCANCODE_W), Controller::Button::Triangle},
        {static_cast<u32>(SDL_SCANCODE_A), Controller::Button::Square},
        {static_cast<u32>(SDL_SCANCODE_S), Controller::Button::X},
        {static_cast<u32>(SDL_SCANCODE_D), Controller::Button::Circle},
        {static_cast<u32>(SDL_SCANCODE_Q), Controller::Button::L1},
        {static_cast<u32>(SDL_SCANCODE_E), Controller::Button::R1},
        {static_cast<u32>(SDL_SCANCODE_Z), Controller::Button::L2},
        {static_cast<u32>(SDL_SCANCODE_C), Controller::Button::R2},
    });

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

//#ifdef SDL_HINT_IME_SHOW_UI
//    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
//#endif

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
                if (emulator.IsPaused()) [[unlikely]] break;
            }

            // check if ready to draw next frame again because the cpu could have hit a breakpoint
            // before reaching the next vblank
            if (emulator.DrawNextFrame()) {
                HandleInput(emulator, controller, display, window);
                display.Update();
                emulator.ResetDrawFrame();
            }

            //display.Throttle(60);

        } else {
            HandleInput(emulator, controller, display, window);
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

void HandleInput(Emulator& emulator, Controller& controller, Display& display, SDL_Window* window) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                emulator.done = true;
                break ;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    emulator.done = true;
                break;
            case SDL_KEYUP:
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_H) emulator.SetPaused(!emulator.IsPaused());
                if (event.key.keysym.scancode == SDL_SCANCODE_R) emulator.Reset();
                if (event.key.keysym.scancode == SDL_SCANCODE_F12) display.SaveScreenshot();

                auto& key_map = controller.GetKeyMap();
                const u32 key = static_cast<u32>(event.key.keysym.scancode);

                if (key_map.find(key) != key_map.end()) {
                    controller.Press(key_map[key]);
                    LogDebug("Pressed valid controller button {}", SDL_GetScancodeName(event.key.keysym.scancode));
                }
                break;
            }
            // drag and drop support
            case SDL_DROPFILE:
                display.HandleDroppedFile(std::string(event.drop.file));
                SDL_free(event.drop.file);
                break;
        }

    }
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
