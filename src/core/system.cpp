#include "system.h"

#include "bus.h"
#include "cdrom.h"
#include "common/asserts.h"
#include "common/config.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "debugger/debugger.h"
#include "debugger/gdb_stub.h"
#include "dma.h"
#include "gpu.h"
#include "interrupt.h"
#include "peripherals.h"
#include "timer/timers.h"

LOG_CHANNEL(System);

System::System() {
    cpu = std::make_unique<CPU::CPU>(this);
    bus = std::make_unique<BUS>(this);
    dma = std::make_unique<DMA>(this);
    gpu = std::make_unique<GPU>(this);
    cdrom = std::make_unique<CDROM>(this);
    interrupt = std::make_unique<InterruptController>(this);
    timers = std::make_unique<TimerController>(this);
    peripherals = std::make_unique<Peripherals>(this);

    debugger = std::make_unique<Debugger>(this);
    stats = std::make_unique<Stats>();

    RecalculateCyclesUntilNextEvent();

    LogInfo("Initialized PSX core");
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
    //peripherals->Reset();

    debugger->Reset();

    // Stats is POD, so just use memset for reset purposes
    std::memset(stats.get(), 0, sizeof(Stats));

    accumulated_cycles = 0;
    cycles_until_next_event = 0;

    RecalculateCyclesUntilNextEvent();

    // can't run both at the same time
    DebugAssert(!(!Config::ps_bin_file_path.empty() && !Config::psexe_file_path.empty()));

    if (!Config::ps_bin_file_path.empty()) {
        // TODO: load game file
    }
    if (!Config::psexe_file_path.empty()) {
        bus->LoadPsExe();
    }

    LogInfo("System reset");
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

    for (auto& event : timed_events) {
        const u32 component_cycles = std::invoke(event.cycles_until_event);

        cycles_until_next_event = std::min(cycles_until_next_event, component_cycles);
    }
}

void System::UpdateComponents(u32 cycles) {
    for (auto& event : timed_events) {
        std::invoke(event.add_cycles, cycles);
    }
}
