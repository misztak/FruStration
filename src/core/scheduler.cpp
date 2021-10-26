#include "scheduler.h"

#include <limits>

#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(Scheduler);

namespace Scheduler {
//namespace {

std::vector<Component> components;

u32 cycles = 0;
u32 cycles_until_next_event = 0;

//}

void Reset() {
    cycles = 0;
    cycles_until_next_event = std::numeric_limits<u32>::max();
}

void AddComponent(Component::Type type, UpdateFunc&& update_func, CyclesUntilNextEventFunc&& cycles_until_next_event_func) {
    components.push_back({type, update_func, cycles_until_next_event_func});
}

void AddCycles(u32 added_cycles) {
    cycles += added_cycles;

    while (cycles >= cycles_until_next_event) {

        if (cycles_until_next_event > 0) {
            // let the components reach the next event
            UpdateComponents(cycles_until_next_event);

            cycles -= cycles_until_next_event;
        }

        // update pending event value
        RecalculateNextEvent();
    }
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

#if 0
    LOG_DEBUG << "Recalculating component timing...";
    LOG_DEBUG << "Current cycles: " << cycles;
#endif

    for (auto& component : components) {
        const u32 comp_cycles = component.CyclesUntilNextEvent();

#if 0
        static const constexpr char* const names[3] = {"CDROM", "GPU", "TIMER"};
        LOG_DEBUG << fmt::format("{}: {} cycles until next event", names[component.type], comp_cycles);
#endif

        cycles_until_next_event = std::min(cycles_until_next_event, comp_cycles);
    }
}

}