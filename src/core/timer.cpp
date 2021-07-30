#include "timer.h"

#include "gpu.h"
#include "interrupt.h"
#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(Timer);

TimerController::TimerController() {}

void TimerController::Init(GPU* g, InterruptController* icontroller) {
    gpu = g;
    interrupt_controller = icontroller;
}

void TimerController::Step(u32 steps, u32 timer_index) {
    auto& timer = timers[timer_index];

    bool send_irq = false;
    if (!timer.mode.sync_enabled) {
        // free run
        Panic("Unimplemented");
    }

    if (timer_index == 0) {
        switch (timer.mode.sync_mode) {
            case 0:
                timer.paused = gpu->in_hblank;
                break;
            case 1:
                if (gpu->in_hblank) timer.counter = 0;
                break;
            case 2:
                if (gpu->in_hblank) timer.counter = 0;
                timer.paused = !gpu->in_hblank;
                break;
            case 3:
                if (timer.paused && gpu->in_hblank) {
                    timer.paused = false;
                    timer.mode.sync_enabled = false;
                }
                break;
        }
    } else if (timer_index == 1) {
        switch (timer.mode.sync_mode) {
            case 0:
                timer.paused = gpu->in_vblank;
                break;
            case 1:
                if (gpu->in_vblank) timer.counter = 0;
                break;
            case 2:
                if (gpu->in_vblank) timer.counter = 0;
                timer.paused = !gpu->in_vblank;
                break;
            case 3:
                if (timer.paused && gpu->in_vblank) {
                    timer.paused = false;
                    timer.mode.sync_enabled = false;
                }
                break;
        }
    } else if (timer_index == 2) {
        switch (timer.mode.sync_mode) {
            case 0:
            case 3:
                timer.paused = true;
                break;
            case 1:
            case 2:
                break;
        }
    }

    u16 last_value = timer.counter;
    // TODO: increment

    if (timer.mode.reset_mode == ResetMode::AfterTarget && (last_value == (timer.target & 0xFFFF))) {
        timer.counter = 0;
        timer.mode.reached_target = true;
        if (timer.mode.irq_on_target) send_irq = true;
    }

    if (timer.mode.reset_mode == ResetMode::AfterMaxValue && (last_value == 0xFFFF)) {
        timer.counter = 0;
        timer.mode.reached_max_value = true;
        if (timer.mode.irq_on_max_value) send_irq = true;
    }

    if (send_irq && timer.mode.allow_irq) {
        // one-shot mode
        if (!timer.mode.irq_repeat_mode)
            timer.mode.allow_irq = false;

        // TODO: in pulse mode Bit10 is zero for a couple of clock cycles while an interrupt occurs

        SendIRQ(timer_index);
    }

    if (send_irq && timer.mode.irq_toggle_mode && timer.mode.irq_repeat_mode)
        timer.mode.allow_irq = !timer.mode.allow_irq;

}

u32 TimerController::Load(u32 address) {
#if 0
    LOG_DEBUG << fmt::format("Load call to Timers [@ 0x{:02X}]", address);
#endif

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    if (timer_address == 0x0) {
        // unimplemented
        Assert(timer_index != 0 && timer_index != 2);
        return timers[timer_index].counter;
    }

    if (timer_address == 0x4) {
        return timers[timer_index].mode.value;
    }

    if (timer_address == 0x8) {
        return timers[timer_index].target;
    }

    Panic("Invalid timer register");
    return 0;
}

void TimerController::Store(u32 address, u32 value) {
    LOG_DEBUG << fmt::format("Store call to Timers [0x{:08X} @ 0x{:08X}]", value, address);

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    auto& timer = timers[timer_index];

    if (timer_address == 0x0) {
        timer.counter = value;
        return;
    }

    if (timer_address == 0x4) {
        // reset counter value
        timer.counter = 0;

        timer.mode.value = value;
        timer.mode.allow_irq = true;

        if (timer_index == 0 || timer_index == 1) {
            if (timer.mode.sync_mode == 3) timer.paused = true;
        } else {
            if (timer.mode.sync_mode == 0 || timer.mode.sync_mode == 3) timer.paused = true;
        }

        return;
    }

    if (timer_address == 0x8) {
        timer.target = value;
        return;
    }

    Panic("Invalid timer register");
}

void TimerController::SendIRQ(u32 index) {
    static constexpr IRQ timer_irq[3] = {
        IRQ::TIMER0,
        IRQ::TIMER1,
        IRQ::TIMER2,
    };

    DebugAssert(index < 3);
    interrupt_controller->Request(timer_irq[index]);
}

TimerController::ClockSource TimerController::GetClockSource(u32 index) {
    auto& timer = timers[index];

    switch (index) {
        case 0:
            if (timer.mode.clock_source == 0 || timer.mode.clock_source == 2)
                return ClockSource::SysClock;
            else
                return ClockSource::DotClock;
        case 1:
            if (timer.mode.clock_source == 0 || timer.mode.clock_source == 2)
                return ClockSource::SysClock;
            else
                return ClockSource::HBlank;
        case 2:
            if (timer.mode.clock_source == 0 || timer.mode.clock_source == 1)
                return ClockSource::SysClock;
            else
                return ClockSource::SysClockEighth;
        default:
            Panic("Invalid timer index");
    }
}

void TimerController::Reset() {
    was_hblank = was_vblank = false;

    for (auto& timer: timers) {
        timer.counter = 0;
        timer.mode.value = 0;
        timer.target = 0;
    }
}
