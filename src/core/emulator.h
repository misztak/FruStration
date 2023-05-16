#pragma once

#include <tuple>

#include "common/config.h"
#include "system.h"
#include "util/types.h"

class Emulator {
public:
    bool LoadBIOS();

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

private:
    System sys;
};
