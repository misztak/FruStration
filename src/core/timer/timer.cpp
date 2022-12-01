#include "timer.h"

#include <limits>

#include "debug_utils.h"
#include "system.h"
#include "types.h"

LOG_CHANNEL(Timer);

Timer::Timer(u32 index, System* system) : index(index), sys(system) {}

bool Timer::Increment(u32 cycles) {
    if (paused) return false;

    DebugAssert(cycles <= CyclesUntilNextIRQ());

    const u32 previous_counter_value = counter;
    counter += cycles;

    bool send_irq = false;

    bool already_reached_target = previous_counter_value >= target;
    if (counter >= target && (!already_reached_target || target == 0)) {
        mode.reached_target = true;
        send_irq |= mode.irq_on_target;
        if (mode.reset_mode == ResetMode::AfterTarget && target > 0) {
            counter %= target;
        }
    }

    if (counter >= MAX_COUNTER) {
        mode.reached_max_value = true;
        send_irq |= mode.irq_on_max_value;
        counter %= MAX_COUNTER;
    }

    auto SendIRQ = [&] {
        if (!pending_irq || mode.irq_repeat_mode) {
            pending_irq = true;
            return true;
        }
        return false;
    };

    if (send_irq) {
        if (mode.irq_toggle_mode) {
            if (mode.irq_repeat_mode || mode.allow_irq) {
                // toggle mode
                mode.allow_irq = !mode.allow_irq;
                // high to low transition
                if (!mode.allow_irq) {
                    return SendIRQ();
                }
            }
        } else {
            // pulse mode, Bit10 always set
            mode.allow_irq = true;
            return SendIRQ();
        }
    }

    return false;
}

u32 Timer::CyclesUntilNextIRQ() {
    u32 cycles = std::numeric_limits<u32>::max();

    if (!paused) {
        if (mode.irq_on_target) {
            const u32 cycles_until_target =
                (counter < target) ? (target - counter) : ((MAX_COUNTER - counter) + target);
            cycles = std::min(cycles, cycles_until_target);
        }

        if (mode.irq_on_max_value) {
            const u32 cycles_until_max_value = MAX_COUNTER - counter;
            cycles = std::min(cycles, cycles_until_max_value);
        }
    }

    return cycles;
}

void Timer::UpdateBlankState(bool entered_blank) {
    // nothing to do if blank state didn't change
    if (gpu_currently_in_blank == entered_blank) return;

    gpu_currently_in_blank = entered_blank;

    if (gpu_currently_in_blank && mode.sync_enabled) {
        switch (mode.sync_mode) {
            case 0: break;
            case 1:
            case 2: counter = 0; break;
            case 3: mode.sync_enabled = false; break;
        }
    }

    UpdatePaused();
}
