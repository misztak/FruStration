#pragma once

struct TimedComponent {
    virtual void Step(u32 cycles) = 0;
    virtual u32 CyclesUntilNextEvent() = 0;
};

