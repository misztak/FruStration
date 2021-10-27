#include "system.h"

#include "bus.h"
#include "cdrom.h"
#include "cpu.h"
#include "debug_utils.h"
#include "debugger.h"
#include "dma.h"
#include "gdb_stub.h"
#include "gpu.h"
#include "interrupt.h"
#include "timer.h"

LOG_CHANNEL(System);

System::System() {
    cpu = std::make_unique<CPU::CPU>(this);
    bus = std::make_unique<BUS>(this);
    dma = std::make_unique<DMA>(this);
    gpu = std::make_unique<GPU>(this);
    cdrom = std::make_unique<CDROM>(this);
    interrupt = std::make_unique<InterruptController>(this);
    timers = std::make_unique<TimerController>(this);
    debugger = std::make_unique<Debugger>(this);

    timed_components[0] = timers.get();
    timed_components[1] = gpu.get();
    timed_components[2] = cdrom.get();

    RecalculateCyclesUntilNextEvent();

    LOG_INFO << "Initialized PSX core";
}

// required to allow forward declaration with unique_ptr
System::~System() = default;

void System::Reset() {
    cpu->Reset();
    bus->Reset();
    dma->Reset();
    gpu->Reset();
    cdrom->Reset();
    interrupt->Reset();
    timers->Reset();
    debugger->Reset();

    accumulated_cycles = 0;
    cycles_until_next_event = 0;

    RecalculateCyclesUntilNextEvent();

    LOG_INFO << "System reset";
}

void System::AddCycles(u32 cycles) {
    accumulated_cycles += cycles;

    // update all components that reached an event
    while (accumulated_cycles >= cycles_until_next_event) {

        if (cycles_until_next_event > 0) {
            // let all the components tick up to the next event
            UpdateComponents(cycles_until_next_event);

            accumulated_cycles -= cycles_until_next_event;
        }

        // recalculate pending event timings
        RecalculateCyclesUntilNextEvent();
    }
}

void System::ForceUpdateComponents() {
    if (accumulated_cycles > 0) {
        // update all components to the current state
        UpdateComponents(accumulated_cycles);

        accumulated_cycles = 0;
    }
}

void System::RecalculateCyclesUntilNextEvent() {
    cycles_until_next_event = MaxCycles;

    for (auto* component : timed_components) {
        const u32 component_cycles = component->CyclesUntilNextEvent();

        cycles_until_next_event = std::min(cycles_until_next_event, component_cycles);
    }
}

void System::UpdateComponents(u32 cycles) {
    for (auto* component: timed_components) {
        component->Step(cycles);
    }
}
