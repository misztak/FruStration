#pragma once

#include <array>
#include <memory>
#include <string>
#include <limits>
#include <functional>

#include "types.h"

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

constexpr u32 TIMED_EVENT_COUNT = 3;

constexpr u32 TIMED_EVENT_TIMERS = 0;
constexpr u32 TIMED_EVENT_GPU = 1;
constexpr u32 TIMED_EVENT_CDROM = 2;

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

    struct TimedEvent {
        std::function<void(u32)> add_cycles = nullptr;
        std::function<u32()> cycles_until_event = nullptr;
    };

    std::array<TimedEvent, TIMED_EVENT_COUNT> timed_events = {};
private:
    void UpdateComponents(u32 cycles);

    u32 accumulated_cycles = 0;
    u32 cycles_until_next_event = 0;
};
