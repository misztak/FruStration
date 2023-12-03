#pragma once

#include <tuple>

#include "common/config.h"
#include "controller.h"
#include "system.h"
#include "util/types.h"

class Emulator {
public:
    bool LoadBIOS();

    void Tick();
    bool DrawNextFrame();
    void ResetDrawFrame();

    bool IsPaused();
    void SetPaused(bool halt);
    void Reset();

    std::tuple<u32, u32, bool> DisplayInfo();
    u8* GetVideoOutput();

    u16* GetVRAM();

    Controller& GetMainController();

    void DrawDebugWindows();
    void StartGDBServer();
    void HandleGDBClientRequest();

    bool done = false;
private:
    System sys;
};
