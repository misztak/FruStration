#pragma once

#include "types.h"
#include "bitfield.h"

class TimerController {
public:
    TimerController();
    void Init();
    void Reset();
    u32 Load(u32 address);
    void Store(u32 address, u32 value);

private:
    struct Timer {
        u32 counter = 0;

        enum class ResetMode: u32 {
            AfterMaxValue, AfterTarget
        };

        enum class ClockSource: u32 {
            SysClock,
            SysClockEighth,
            DotClock,
            HBlank,
        };

        union {
            BitField<u32, bool, 0, 1> sync_enabled;
            BitField<u32, u32, 1, 2> sync_mode;
            BitField<u32, ResetMode, 3, 1> reset_mode;
            BitField<u32, bool, 4, 1> irq_on_target;
            BitField<u32, bool, 5, 1> irq_on_max_value;
            BitField<u32, bool, 6, 1> irq_repeat_mode;
            BitField<u32, bool, 7, 1> irq_toggle_mode;
            BitField<u32, ClockSource, 8, 2> clock_source;
            BitField<u32, bool, 10, 1> interrupt_req;
            BitField<u32, bool, 11, 1> reached_target;
            BitField<u32, bool, 12, 1> reached_max_value;

            u32 value = 0;
        } mode;

        u32 target = 0;
    };

    Timer timers[3] = {};
};
