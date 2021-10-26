#pragma once

#include <vector>
#include <limits>
#include <functional>

#include "types.h"

static constexpr u32 MaxCycles = std::numeric_limits<u32>::max();

using UpdateFunc = std::function<void (u32)>;
using CyclesUntilNextEventFunc = std::function<u32 ()>;

struct Component {
    enum Type {
        CDROM, GPU, TIMER,
    };
    Type type;

    UpdateFunc Update = nullptr;
    CyclesUntilNextEventFunc CyclesUntilNextEvent = nullptr;
};

namespace Scheduler {

void Reset();
void AddComponent(Component::Type type, UpdateFunc&& update_func, CyclesUntilNextEventFunc&& cycles_until_next_event_func);

void AddCycles(u32 cycles);
void ForceUpdate();
void UpdateComponents(u32 cycles_to_update);

void RecalculateNextEvent();

}