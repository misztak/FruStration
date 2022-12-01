#include "timer.h"

#include "interrupt.h"
#include "system.h"

SystemTimer::SystemTimer(u32 index, System *system) : Timer(index, system) {}

void SystemTimer::Step(u32 cycles) {
    u32 ticks;
    if (!IsUsingSystemClock()) {
        ticks = (cycles + div_8_remainder) / 8;
        div_8_remainder = (cycles + div_8_remainder) % 8;
    } else {
        ticks = cycles;
    }

    bool reached_irq = Increment(ticks);
    if (reached_irq) {
        sys->interrupt->Request(IRQ::TIMER2);

        if (mode.sync_enabled && StopAtCurrentValue()) {
            // stop the counter permanently
            counter = (mode.irq_on_target && mode.reached_target) ? target : MAX_COUNTER;
            paused = true;
        }
    }
}

u32 SystemTimer::CyclesUntilNextEvent() {
    const u32 remaining_cycles = CyclesUntilNextIRQ();

    if (!IsUsingSystemClock() && remaining_cycles != MaxCycles) {
        return remaining_cycles * 8 - div_8_remainder;
    } else {
        return remaining_cycles;
    }
}

void SystemTimer::UpdatePaused() {
    if (mode.sync_enabled) {
        // counter is stopped 'forever' in syncmode 0 and 3
        // otherwise free run
        paused = paused && StopAtCurrentValue();
    } else {
        // free run
        paused = false;
    }
}

bool SystemTimer::IsUsingSystemClock() const {
    return (mode.clock_source & 0x2) == 0;
}

bool SystemTimer::StopAtCurrentValue() const {
    return (mode.sync_mode % 3) == 0;
}
