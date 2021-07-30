#pragma once

#include "types.h"
#include "bitfield.h"

class GPU;
class InterruptController;

class TimerController {
public:
    TimerController();
    void Init(GPU* gpu, InterruptController* icontroller);
    void Reset();
    void Step(u32 steps, u32 timer_index);
    u32 Load(u32 address);
    void Store(u32 address, u32 value);

private:
    enum class ResetMode: u32 {
        AfterMaxValue, AfterTarget
    };

    enum class ClockSource: u32 {
        SysClock,
        SysClockEighth,
        DotClock,
        HBlank,
    };

    ClockSource GetClockSource(u32 index);
    void SendIRQ(u32 index);

    bool was_hblank = false, was_vblank = false;

    struct Timer {
        u32 counter = 0;

        union {
            BitField<u32, bool, 0, 1> sync_enabled;
            BitField<u32, u32, 1, 2> sync_mode;
            BitField<u32, ResetMode, 3, 1> reset_mode;
            BitField<u32, bool, 4, 1> irq_on_target;
            BitField<u32, bool, 5, 1> irq_on_max_value;
            BitField<u32, bool, 6, 1> irq_repeat_mode;
            BitField<u32, bool, 7, 1> irq_toggle_mode;
            BitField<u32, u32, 8, 2> clock_source;
            BitField<u32, bool, 10, 1> allow_irq;
            BitField<u32, bool, 11, 1> reached_target;
            BitField<u32, bool, 12, 1> reached_max_value;

            u32 value = 0;
        } mode;

        u32 target = 0;

        bool paused = false;
    };

    Timer timers[3] = {};

    GPU* gpu = nullptr;
    InterruptController* interrupt_controller = nullptr;
};
