#pragma once

#include <string>

#include "system.h"
#include "types.h"

class Emulator {
public:
    bool LoadBIOS(const std::string& bios_path);

    void Tick();
    bool DrawNextFrame();
    void ResetDrawFrame();

    bool IsHalted();
    void SetHalt(bool halt);
    void Reset();

    bool In24BPPMode();
    u8* GetVideoOutput();

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

    // end of config

    u16* GetVRAM();

private:
    System sys;
};
