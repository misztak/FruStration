#pragma once

#include "types.h"
#include "bitfield.h"

class GPU;
class InterruptController;

constexpr u32 MAX_COUNTER = 0xFFFF;

constexpr static u32 TMR0 = 0;
constexpr static u32 TMR1 = 1;
constexpr static u32 TMR2 = 2;

class TimerController {
public:
    TimerController();
    void Init(GPU* gpu, InterruptController* icontroller);
    void Reset();

    u32 Load(u32 address);
    u32 Peek(u32 address);
    void Store(u32 address, u32 value);

    void StepTmp(u32 cycles);
    u32 CyclesUntilNextEvent();

    void DrawTimerState(bool* open);

    enum class ResetMode: u32 {
        AfterMaxValue, AfterTarget
    };

    enum class ClockSource: u32 {
        SysClock,
        SysClockEighth,
        DotClock,
        HBlank,
    };

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
        bool pending_irq = false;

        bool gpu_currently_in_blank = false;

        bool IsUsingSystemClock() const {
            return mode.clock_source % 2 == 0;
        }

        bool Increment(u32 cycles);

        void UpdateOnBlankFlip(u32 index, bool entered_blank);
        void UpdatePaused(u32 index);
        u32 CyclesUntilNextIRQ();
    };

    Timer timers[3] = {};
private:
    u32 div_8_remainder = 0;

    GPU* gpu = nullptr;
    InterruptController* interrupt_controller = nullptr;
};
