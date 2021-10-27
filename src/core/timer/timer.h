#pragma once

#include "bitfield.h"
#include "types.h"

constexpr u32 MAX_COUNTER = 0xFFFF;

constexpr static u32 TMR0 = 0;
constexpr static u32 TMR1 = 1;
constexpr static u32 TMR2 = 2;

enum class ResetMode: u32 {
    AfterMaxValue, AfterTarget
};

class System;

// timer.cpp
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

    const u32 index;
    System* sys = nullptr;

    Timer() = delete;
    Timer(u32 index, System* system);

    bool IsUsingSystemClock() const {
        return mode.clock_source % 2 == 0;
    }

    bool Increment(u32 cycles);

    u32 CyclesUntilNextIRQ();

    void UpdateBlankState(bool entered_blank);

    virtual void Step(u32 cycles) = 0;
    virtual u32 CyclesUntilNextEvent() = 0;
    virtual void UpdatePaused() = 0;
};

// timer_blank.cpp
struct BlankTimer : public Timer {
    BlankTimer(u32 index, System* system);
    void Step(u32 cycles) override;
    u32 CyclesUntilNextEvent() override;
    void UpdatePaused() override;
};

// timer_system.cpp
struct SystemTimer : public Timer {
    bool IsUsingSysClockEighth() const;
    bool StopAtCurrentValue() const;
    u32 div_8_remainder = 0;

    SystemTimer(u32 index, System* system);
    void Step(u32 cycles) override;
    u32 CyclesUntilNextEvent() override;
    void UpdatePaused() override;
};
