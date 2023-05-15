#pragma once

#include <string>
#include <tuple>

#include "system.h"
#include "util/types.h"

class Emulator {
public:
    bool LoadBIOS(const std::string& bios_path);

    void Tick();
    bool DrawNextFrame();
    void ResetDrawFrame();

    bool IsHalted();
    void SetHalt(bool halt);
    void Reset();

    std::tuple<u32, u32, bool> DisplayInfo();
    u8* GetVideoOutput();

    u16* GetVRAM();

    bool done = false;

    void DrawDebugWindows();
    void StartGDBServer();
    void HandleGDBClientRequest();

    // TODO: actual config system
    // bootleg config
    bool draw_mem_viewer = false;
    bool draw_cpu_state = true;
    bool draw_gpu_state = true;
    bool draw_debugger = true;
    bool draw_timer_state = true;

    bool cfg_gdb_server_enabled = false;

    std::string bios_path = "../bios/SCPH1001.BIN";
    const std::string imgui_ini_path = "../imgui.ini";
    // end of config

private:
    System sys;
};
