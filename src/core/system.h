#pragma once

#include <memory>
#include <string>

#include "types.h"

namespace CPU {
class CPU;
}

class BUS;
class DMA;
class GPU;
class InterruptController;
class TimerController;
class Debugger;

class System {
public:
    System();
    ~System();
    void Init();
    bool LoadBIOS(const std::string& bios_path);
    void RunFrame();
    void Run();
    void VBlank();

    bool IsHalted();
    void SetHalt(bool halt);
    void Reset();

    void DrawDebugWindows();

    // TODO: manage stuff like this through a config system
    bool draw_mem_viewer = false;
    bool draw_cpu_state = true;
    bool draw_gpu_state = true;
    bool draw_debugger = true;
    bool draw_disassembler = false;

    u16* GetVRAM();
private:
    std::unique_ptr<CPU::CPU> cpu;
    std::unique_ptr<BUS> bus;
    std::unique_ptr<DMA> dma;
    std::unique_ptr<GPU> gpu;
    std::unique_ptr<InterruptController> interrupt;
    std::unique_ptr<TimerController> timers;
    std::unique_ptr<Debugger> debugger;
};
