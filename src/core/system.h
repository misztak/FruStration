#pragma once

#include <array>
#include <memory>
#include <string>
#include <limits>

#include "types.h"
#include "timed_component.h"

namespace CPU {
class CPU;
}

class BUS;
class DMA;
class GPU;
class CDROM;
class InterruptController;
class TimerController;
class Debugger;

constexpr u32 MaxCycles = std::numeric_limits<u32>::max();

class System {
public:
    System();
    ~System();
    void Reset();

    void AddCycles(u32 cycles);
    void ForceUpdateComponents();
    void RecalculateCyclesUntilNextEvent();

    std::unique_ptr<CPU::CPU> cpu;
    std::unique_ptr<BUS> bus;
    std::unique_ptr<DMA> dma;
    std::unique_ptr<GPU> gpu;
    std::unique_ptr<CDROM> cdrom;
    std::unique_ptr<InterruptController> interrupt;
    std::unique_ptr<TimerController> timers;
    std::unique_ptr<Debugger> debugger;

private:
    static constexpr u32 TIMED_COMPONENT_COUNT = 3;

    void UpdateComponents(u32 cycles);

    u32 accumulated_cycles = 0;
    u32 cycles_until_next_event = 0;

    std::array<TimedComponent*, TIMED_COMPONENT_COUNT> timed_components = {};
};
