#include "display.h"

#include <algorithm>
#include <cmath>
#include <thread>
#include <filesystem>
#include <optional>

#include "font_jetbrains_mono.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#ifdef USE_NFD
#include "nfd.h"
#endif

#include "common/asserts.h"
#include "common/config.h"
#include "common/log.h"
#include "emulator.h"

namespace fs = std::filesystem;

LOG_CHANNEL(Display);

#ifdef USE_NFD
static std::optional<std::string> OpenFileDialog(const char* filter_list = nullptr) {
    nfdchar_t* out_path = nullptr;
    nfdresult_t result = NFD_OpenDialog(filter_list, nullptr, &out_path);

    if (result == NFD_OKAY) {
        LogInfo("Selected file {}", out_path);
        return { fs::path(out_path).string() };
    } else {
        if (result == NFD_ERROR) LogWarn("Failed to open file: {}", NFD_GetError());
        return std::nullopt;
    }
}
#endif

static void UpdateWindowTitle(SDL_Window* window) {
    std::string file_name = "no file";
    if (!Config::ps_bin_file_path.empty())
        file_name = fs::path(Config::ps_bin_file_path).filename().string();
    else if (!Config::psexe_file_path.empty())
        file_name = fs::path(Config::psexe_file_path).filename().string();

    std::string new_window_title = fmt::format("frustration - [{}]", file_name);
    SDL_SetWindowTitle(window, new_window_title.c_str());
}


Display::Display() = default;

bool Display::Init(Emulator* system, SDL_Window* win, SDL_GLContext context, const char* glsl_version) {
    emu = system;
    window = win;
    gl_context = context;
    DebugAssert(emu);
    DebugAssert(window);
    DebugAssert(gl_context);

    UpdateWindowTitle(window);

    // activate drag and drop support
    // TODO: make this a config?
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    // set scale factor
    // TODO: implement proper host display scaling logic
    int display = SDL_GetWindowDisplayIndex(window);
    float dpi = 96.0f;
    if (SDL_GetDisplayDPI(display, &dpi, nullptr, nullptr) != 0) {
        LogCrit("Failed to get window dpi");
        return false;
    }
    float scale_factor = std::min(2.f, dpi / 96.0f);
    Assert(scale_factor > 1.f);

    // print host display info
    LogInfo("Number of displays: {}", SDL_GetNumVideoDisplays());
    LogInfo("Current display: {}", SDL_GetWindowDisplayIndex(window));
    LogInfo("DPI: {} (default=96.0f)", dpi);
    LogInfo("Scale factor: {:.2}", scale_factor);
    LogInfo("SDL2 version {}.{}.{}", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    LogInfo("OpenGL version {} (Vendor: {})", (char*)glGetString(GL_VERSION), (char*)glGetString(GL_VENDOR));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // add scaled font
    io.Fonts->AddFontFromMemoryCompressedTTF(jetbrains_regular_compressed_data, jetbrains_regular_compressed_size,
                                             16.0f * scale_factor);

    const s32 scale_int = std::ceil(scale_factor);
    SDL_SetWindowSize(window, WIDTH * scale_int, HEIGHT * scale_int);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0));

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

    // Menu Bar (Top)

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {

#ifdef USE_NFD
            if (ImGui::MenuItem("Open...", "CTRL-O")) {
                auto open_result = OpenFileDialog("bin,BIN");
                if (open_result.has_value()) {
                    Config::ps_bin_file_path = open_result.value();
                    UpdateWindowTitle(window);
                    emu->Reset();
                }
            }
            if (ImGui::MenuItem("Open EXE file...")) {
                auto open_result = OpenFileDialog("exe,EXE,psexe");
                if (open_result.has_value()) {
                    Config::psexe_file_path = open_result.value();
                    UpdateWindowTitle(window);
                    emu->Reset();
                }
            }
            if (ImGui::MenuItem("Open BIOS file...")) {
                auto open_result = OpenFileDialog("bin,BIN");
                if (open_result.has_value()) Config::bios_path.Set(open_result.value());
            }
#endif

            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) emu->done = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings")) {
            bool emu_paused = emu->IsPaused();
            if (ImGui::MenuItem("Pause", "H", &emu_paused)) emu->SetPaused(emu_paused);
            if (ImGui::MenuItem("Reset", "R")) emu->Reset();
            ImGui::Separator();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
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

    // Status Bar (bottom)

    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    ImGuiWindowFlags stat_bar_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();

    const ImVec4 orange(.8f, .6f, .3f, 1.f);
    const ImVec4 green(.0688, .860, .108, 1.0);

    const ImGuiTableFlags stat_bar_table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_SizingFixedFit;

    const float viewport_width = viewport->Size.x;

    // precalculate the space needed for the right-aligned part of the status bar
    const char* fps_dummy_string = " 00.0 FPS ";
    const char* frame_dummy_string = " Frame: 00.000 ms   ";

    float rights_stat_bar_size = ImGui::CalcTextSize(fps_dummy_string).x + ImGui::CalcTextSize(frame_dummy_string).x;
    float left_stat_bar_size = 0.0f;
    if (viewport_width > rights_stat_bar_size) left_stat_bar_size = viewport_width - rights_stat_bar_size;

    if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, stat_bar_flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginTable("status_bar_table", 4, stat_bar_table_flags, ImVec2(left_stat_bar_size, 0.0f))) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (emu->IsPaused()) {
                    ImGui::TextColored(orange, " Paused ");
                } else {
                    ImGui::TextColored(green, " Running ");
                }

                ImGui::TableNextColumn();
                ImGui::Text(" VSYNC %s ", vsync_enabled ? "ON" : "OFF");

                ImGui::TableNextColumn();
                ImGui::Text(" Renderer: SW ");

                ImGui::TableNextColumn();
                ImGui::Text(" %s ", fs::path(Config::bios_path.Get()).filename().c_str());

                ImGui::EndTable();
            }

            if (ImGui::BeginTable("status_bar_table_right", 2, stat_bar_table_flags)) {
                //ImGui::TableSetupColumn("status_bar_table_right_col", 0);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text(" %2.1f FPS ", ImGui::GetIO().Framerate);

                ImGui::TableNextColumn();
                ImGui::Text(" Frame: %2.3f ms ", 1000.0f / ImGui::GetIO().Framerate);

                ImGui::EndTable();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
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

        ImGui::SetNextWindowSize(ImVec2(hres + 25, vres + 60));
        ImGui::Begin("Video", nullptr, ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((void*)(intptr_t)output_tex_handler, ImVec2(hres, vres));
        ImGui::End();
    }

    emu->DrawDebugWindows();

    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    if (!dropped_file.empty()) ImGui::OpenPopup("Dropped File");
    DrawDragAndDropPopup();
}

void Display::DrawDragAndDropPopup() {
    if (ImGui::BeginPopupModal("Dropped File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string dropped_filename = fs::path(dropped_file).filename();
        ImGui::Text("Please select the file type for %s", dropped_filename.c_str());
        ImGui::Separator();

        static int type = 0;
        ImGui::RadioButton("PS EXE", &type, 0);
        ImGui::RadioButton("DISC IMAGE", &type, 1);
        ImGui::RadioButton("BIOS BIN", &type, 2);
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (type == 0) {
                std::string old_file = Config::psexe_file_path;
                Config::psexe_file_path = dropped_file;
                bool success = emu->LoadPsExe();
                if (!success) Config::psexe_file_path = old_file;
            }
            if (type == 1) {
                Config::ps_bin_file_path = dropped_file;
                // TODO: load the actual file
            }
            if (type == 2) {
                Config::bios_path.Set(dropped_file);
                emu->LoadBIOS();
            }

            emu->Reset();
            UpdateWindowTitle(window);

            dropped_file.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            dropped_file.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Display::Render() {
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
}

void Display::Update() {
    Draw();
    Render();
}

void Display::HandleDroppedFile(std::string file) {
    // don't allow nested file drop popups
    Assert(dropped_file.empty());
    dropped_file = std::move(file);

    LogInfo("Dropped file '{}'", dropped_file);
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
