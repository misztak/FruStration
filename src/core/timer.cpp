#include "timer.h"

#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(Timer);

TimerController::TimerController() {}

void TimerController::Init() {}

void TimerController::Reset() {
    for (auto& timer: timers) {
        timer.counter = 0;
        timer.mode.value = 0;
        timer.target = 0;
    }
}

u32 TimerController::Load(u32 address) {
#if 0
    LOG_DEBUG << fmt::format("Load call to Timers [@ 0x{:02X}]", address);
#endif

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    if (timer_address == 0x0) {
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

    if (timer_address == 0x0) {
        timers[timer_index].counter = value;
        return;
    }

    if (timer_address == 0x4) {
        timers[timer_index].mode.value = value;
        return;
    }

    if (timer_address == 0x8) {
        timers[timer_index].target = value;
        return;
    }

    Panic("Invalid timer register");
}
