#include "timer.h"

#include "interrupt.h"
#include "system.h"

BlankTimer::BlankTimer(u32 index, System *system) : Timer(index, system) {}

void BlankTimer::Step(u32 cycles) {
    if (IsUsingSystemClock()) {
        bool reached_irq = Increment(cycles);
        if (reached_irq) {
            sys->interrupt->Request(index == TMR0 ? IRQ::TIMER0 : IRQ::TIMER1);
        }
    }
}

u32 BlankTimer::CyclesUntilNextEvent() {
    if (IsUsingSystemClock()) {
        return CyclesUntilNextIRQ();
    } else {
        return MaxCycles;
    }
}

void BlankTimer::UpdatePaused() {
    if (mode.sync_enabled) {
        switch (mode.sync_mode) {
            case 0: paused = gpu_currently_in_blank; break;
            case 1: paused = false; break;
            case 2:
            case 3: paused = !gpu_currently_in_blank; break;
        }
    } else {
        // free run
        paused = false;
    }
}

bool BlankTimer::IsUsingSystemClock() const {
    return mode.clock_source % 2 == 0;
}
