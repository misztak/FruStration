#include "scheduler.h"

#include <limits>

#include "macros.h"

LOG_CHANNEL(SCHEDULER);

namespace Scheduler {
namespace {

std::vector<Component> components;

u32 cycles = 0;
u32 cycles_until_next_event = 0;

}

void Reset() {
    cycles = 0;
    cycles_until_next_event = std::numeric_limits<u32>::max();
}

void AddComponent(Component::Type type, UpdateFunc&& update_func, CyclesUntilNextEventFunc&& cycles_until_next_event_func) {
    components.push_back({type, update_func, cycles_until_next_event_func});
}

static void FlushCycles() {
    while (cycles >= cycles_until_next_event) {
        cycles -= cycles_until_next_event;

        // let the components reach the next event
        UpdateComponents(cycles_until_next_event);

        // update pending event value
        RecalculateNextEvent();
    }
}

void AddCycles(u32 added_cycles) {
    cycles += added_cycles;
    FlushCycles();
}

void ForceUpdate() {
    if (cycles > 0) {
        UpdateComponents(cycles);
        // manually reset cycle counter
        cycles = 0;
    }
}

void UpdateComponents(u32 cycles_to_update) {
    for (auto& component : components) {
        component.Update(cycles_to_update);
    }
}

void RecalculateNextEvent() {
    cycles_until_next_event = std::numeric_limits<u32>::max();

    for (auto& component : components) {
        cycles_until_next_event = std::min(cycles_until_next_event, component.CyclesUntilNextEvent());
    }
}

}