#pragma once

#include "timer.h"
#include "util/bitfield.h"
#include "util/types.h"

class System;

class TimerController {
public:
    TimerController(System* system);
    void Reset();

    u32 Load(u32 address);
    u32 Peek(u32 address);
    void Store(u32 address, u32 value);

    void Step(u32 cycles);
    u32 CyclesUntilNextEvent();

    void DrawTimerState(bool* open);

    BlankTimer dot_timer;
    BlankTimer hblank_timer;
    SystemTimer system_timer;

    Timer* timers[3] = {};

private:
    static constexpr u32 MODE_WRITE_MASK = 0b1110001111111111;

    System* sys = nullptr;
};
