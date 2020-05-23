#include "display.h"

#include <thread>

#include "font_jetbrains_mono.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "types.h"

Display::Display() {}

bool Display::Init(SDL_Window* win, SDL_GLContext context, const char* glsl_version) {
    window = win;
    gl_context = context;
    Assert(window);
    Assert(gl_context);

    // set scale factor
    // TODO: platform-dependent default DPI values
    int display = SDL_GetWindowDisplayIndex(window);
    float dpi = 96.0f;
    if (SDL_GetDisplayDPI(display, &dpi, nullptr, nullptr) != 0) {
        fprintf(stderr, "Failed to get window dpi\n");
        return false;
    }
    float scale_factor = dpi / 96.0f;
    printf("Using scale factor %f\n", scale_factor);

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
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(jetbrains_regular_compressed_data_base85, 15.0f * scale_factor);
    // scale everything
    io.DisplayFramebufferScale.x = scale_factor;
    io.DisplayFramebufferScale.y = scale_factor;
    style.ScaleAllSizes(scale_factor);

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    start = std::chrono::system_clock::now();
    end = std::chrono::system_clock::now();
    return true;
}

void Display::Draw(bool* done, bool vsync) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open", "CTRL-O", false, false);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) *done = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Stats", nullptr, &show_stats_window);
            ImGui::MenuItem("Demo", nullptr, &show_demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (show_stats_window) {
        ImGui::Begin("Stats", &show_stats_window, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Vsync: %s", vsync ? "enabled" : "disabled");
        ImGui::End();
    }

    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
}

void Display::Render() {
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
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
