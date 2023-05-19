#pragma once

#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <string>

#include "util/types.h"

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
    u32 GetCyclesUntilNextEvent() const { return cycles_until_next_event; }

    std::unique_ptr<CPU::CPU> cpu;
    std::unique_ptr<BUS> bus;
    std::unique_ptr<DMA> dma;
    std::unique_ptr<GPU> gpu;
    std::unique_ptr<CDROM> cdrom;
    std::unique_ptr<InterruptController> interrupt;
    std::unique_ptr<TimerController> timers;
    std::unique_ptr<Debugger> debugger;

    enum class TimedEvent : u32 { Timer = 0, GPU = 1, CDROM = 2, Count = 3 };

    struct TimedEventCallbacks {
        std::function<void(u32)> add_cycles = nullptr;
        std::function<u32()> cycles_until_event = nullptr;
    };

    std::array<TimedEventCallbacks, static_cast<u32>(TimedEvent::Count)> timed_events = {};

    void RegisterEvent(System::TimedEvent event_index, std::function<void(u32)>&& add_cycles_fn,
                       std::function<u32()>&& cycles_until_event_fn) {
        timed_events[static_cast<u32>(event_index)] = {add_cycles_fn, cycles_until_event_fn};
    }
private:
    void UpdateComponents(u32 cycles);

    u32 accumulated_cycles = 0;
    u32 cycles_until_next_event = 0;
};
