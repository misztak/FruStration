#include "display.h"

#include <algorithm>
#include <cmath>
#include <thread>

#include "font_jetbrains_mono.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "nfd.h"

#include "common/asserts.h"
#include "common/config.h"
#include "common/log.h"
#include "emulator.h"

LOG_CHANNEL(Display);

Display::Display() {}

bool Display::Init(Emulator* system, SDL_Window* win, SDL_GLContext context, const char* glsl_version) {
    emu = system;
    window = win;
    gl_context = context;
    Assert(emu);
    Assert(window);
    Assert(gl_context);

    // set scale factor
    // TODO: platform-dependent default DPI values
    int display = SDL_GetWindowDisplayIndex(window);
    float dpi = 96.0f;
    if (SDL_GetDisplayDPI(display, &dpi, nullptr, nullptr) != 0) {
        LogCrit("Failed to get window dpi");
        return false;
    }
    float scale_factor = std::min(2.f, dpi / 96.0f);
    LogInfo("Using scale factor {:.2}", scale_factor);

    // int scaled_x = static_cast<int>(std::floor(scale_factor * DEFAULT_W));
    // int scaled_y = static_cast<int>(std::floor(scale_factor * DEFAULT_H));
    // SDL_SetWindowSize(window, scaled_x, scaled_y);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // add scaled font
    io.Fonts->AddFontFromMemoryCompressedTTF(jetbrains_regular_compressed_data, jetbrains_regular_compressed_size,
                                             15.0f * scale_factor);
    // scale imgui
    io.DisplayFramebufferScale.x = scale_factor;
    io.DisplayFramebufferScale.y = scale_factor;
    style.ScaleAllSizes(scale_factor);
    // scale SDL_Window
    const s32 scale_int = std::ceil(scale_factor);
    SDL_SetWindowSize(window, WIDTH * scale_int, HEIGHT * scale_int);

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // build the vram texture (always 15BPP)
    glGenTextures(1, &vram_tex_handler);
    glBindTexture(GL_TEXTURE_2D, vram_tex_handler);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, system->GetVRAM());
    GLenum status;
    if ((status = glGetError()) != GL_NO_ERROR) {
        LogCrit("OpenGL error {} during texture creation", status);
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // build the output texture (can be either 15BPP or 24BPP)
    glGenTextures(1, &output_tex_handler);
    glBindTexture(GL_TEXTURE_2D, output_tex_handler);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);
    if ((status = glGetError()) != GL_NO_ERROR) {
        LogCrit("OpenGL error {} during texture creation", status);
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    start = std::chrono::system_clock::now();
    end = std::chrono::system_clock::now();
    return true;
}

void Display::Draw() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "CTRL-O")) {
                nfdchar_t* out_path = nullptr;
                nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &out_path);
                if (result == NFD_OKAY) {
                    LogInfo("Selected file {}", out_path);
                } else if (result == NFD_ERROR) {
                    LogWarn("Failed to open file: {}", NFD_GetError());
                }
            }
            ImGui::Separator();
            bool emu_paused = emu->IsHalted();
            if (ImGui::MenuItem("Pause", "H", &emu_paused)) emu->SetHalt(emu_paused);
            if (ImGui::MenuItem("Reset")) emu->Reset();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) emu->done = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Stats", nullptr, &show_stats_window);
            ImGui::MenuItem("CPU Stats", nullptr, &Config::draw_cpu_state);
            ImGui::MenuItem("GPU Stats", nullptr, &Config::draw_gpu_state);
            ImGui::MenuItem("Timer Stats", nullptr, &Config::draw_timer_state);
            ImGui::MenuItem("Debugger", nullptr, &Config::draw_debugger);
            ImGui::MenuItem("Mem Editor", nullptr, &Config::draw_mem_viewer);
            ImGui::MenuItem("Demo", nullptr, &show_demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // HACK: place emulator into a separate window to delay dealing with OpenGL
    {
        glBindTexture(GL_TEXTURE_2D, vram_tex_handler);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, emu->GetVRAM());

        ImGui::SetNextWindowSize(ImVec2(1024 + 25, 512 + 60));
        ImGui::Begin("VRAM", nullptr, ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((void*)(intptr_t)vram_tex_handler, ImVec2(1024, 512));
        ImGui::End();
    }

    // video output
    {
        auto [hres, vres, in_24_bpp_mode] = emu->DisplayInfo();

        glBindTexture(GL_TEXTURE_2D, output_tex_handler);
        if (in_24_bpp_mode) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)hres, (GLsizei)vres, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         emu->GetVideoOutput());
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)hres, (GLsizei)vres, 0, GL_RGBA,
                         GL_UNSIGNED_SHORT_1_5_5_5_REV, emu->GetVideoOutput());
        }

        ImGui::SetNextWindowSize(ImVec2(640 + 25, 480 + 60));
        ImGui::Begin("Video", nullptr, ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((void*)(intptr_t)output_tex_handler, ImVec2(hres, vres));
        ImGui::End();
    }

    if (show_stats_window) {
        ImGui::Begin("Stats", &show_stats_window, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Vsync: %s", vsync_enabled ? "enabled" : "disabled");
        ImGui::End();
    }

    emu->DrawDebugWindows();

    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
}

void Display::Render() {
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(window);
}

void Display::Update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) emu->done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window))
            emu->done = true;
        if (event.type == SDL_KEYUP) {
            if (event.key.keysym.scancode == SDL_SCANCODE_H) emu->SetHalt(!emu->IsHalted());
        }
    }

    Draw();
    Render();
}

void Display::Throttle(u32 fps) {
    const double frame_time_micro = (1.0 / fps) * 1e6;
    start = std::chrono::system_clock::now();
    std::chrono::duration<double, std::micro> used_time = start - end;
    if (used_time.count() < frame_time_micro) {
        std::chrono::duration<double, std::micro> remaining_time(frame_time_micro - used_time.count());
        auto remaining_time_long = std::chrono::duration_cast<std::chrono::microseconds>(remaining_time);
        std::this_thread::sleep_for(std::chrono::microseconds(remaining_time_long.count()));
    }
    end = std::chrono::system_clock::now();
}
