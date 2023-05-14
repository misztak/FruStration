#include "timers.h"

#include "imgui.h"

#include "common/asserts.h"
#include "common/log.h"
#include "gpu.h"
#include "interrupt.h"
#include "system.h"

LOG_CHANNEL(Timer);

TimerController::TimerController(System* system)
    : dot_timer(TMR0, system), hblank_timer(TMR1, system), system_timer(TMR2, system), sys(system) {

    timers[0] = &dot_timer;
    timers[1] = &hblank_timer;
    timers[2] = &system_timer;

    // register timed event
    sys->RegisterEvent(
        System::TimedEvent::Timer, [this](u32 cycles) { Step(cycles); }, [this]() { return CyclesUntilNextEvent(); });
}

void TimerController::Step(u32 cycles) {
    for (auto& timer : timers) timer->Step(cycles);
}

u32 TimerController::CyclesUntilNextEvent() {
    u32 min_cycles = std::numeric_limits<u32>::max();

    for (auto& timer : timers) {
        min_cycles = std::min(min_cycles, timer->CyclesUntilNextEvent());
    }

    return min_cycles;
}

u32 TimerController::Load(u32 address) {
#if 0
    LOG_DEBUG << fmt::format("Load call to Timers [@ 0x{:02X}]", address);
#endif

    sys->ForceUpdateComponents();

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    Assert(timer_index <= 2);

    u32 value;

    if (timer_address == 0x0) {
        value = timers[timer_index]->counter;
    }

    else if (timer_address == 0x4)
    {
        // reached flags get reset after read
        value = timers[timer_index]->mode.value;
        timers[timer_index]->mode.reached_target = false;
        timers[timer_index]->mode.reached_max_value = false;
    }

    else if (timer_address == 0x8)
    {
        value = timers[timer_index]->target;
    }

    else
    {
        Panic("Invalid timer register");
    }

    sys->RecalculateCyclesUntilNextEvent();

    return value;
}

void TimerController::Store(u32 address, u32 value) {
    LogDebug("Store call to Timers [0x{:08X} @ 0x{:08X}]", value, address);

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    Assert(timer_index <= 2);

    sys->ForceUpdateComponents();

    auto& timer = timers[timer_index];

    if (timer_address == 0x0) {

        timer->counter = value;

    } else if (timer_address == 0x4) {

        // reset IRQ
        timer->pending_irq = false;

        // reset counter value
        timer->counter = 0;

        timer->mode.value = (timer->mode.value & ~MODE_WRITE_MASK) | (value & MODE_WRITE_MASK);

        if (timer->mode.irq_toggle_mode) {
            timer->mode.allow_irq = true;
        }

        timer->UpdatePaused();

    } else if (timer_address == 0x8) {

        timer->target = value;

    } else {
        Panic("Invalid timer register");
    }

    sys->RecalculateCyclesUntilNextEvent();
}

u32 TimerController::Peek(u32 address) {
    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    if (timer_address == 0x0) {
        return timers[timer_index]->counter;
    }
    if (timer_address == 0x4) {
        return timers[timer_index]->mode.value;
    }
    if (timer_address == 0x8) {
        return timers[timer_index]->target;
    }
    return 0;
}

void TimerController::Reset() {
    for (auto& timer : timers) {
        timer->counter = 0;
        timer->mode.value = 0;
        timer->target = 0;
        timer->paused = false;
        timer->pending_irq = false;
        timer->gpu_currently_in_blank = false;
    }

    system_timer.div_8_remainder = 0;
}

void TimerController::DrawTimerState(bool* open) {
    ImGui::Begin("Timer State", open);
    ImGui::Columns(4);

    const ImVec4 white(1.0, 1.0, 1.0, 1.0);
    const ImVec4 grey(0.5, 0.5, 0.5, 1.0);

    const auto ClockSourceName = [](Timer* timer) {
        if (timer->IsUsingSystemClock()) return "System Clock";
        switch (timer->index) {
            case TMR0: return "Dotclock";
            case TMR1: return "HBlank";
            case TMR2: return "System Clock / 8";
        }
        return "XXX";
    };

    ImGui::Text("Status");
    ImGui::Text("Counter");
    ImGui::Text("Target");

    ImGui::Text("Sync");
    ImGui::Text("Sync mode");

    ImGui::Text("IRQ on target");
    ImGui::Text("IRQ on max value");

    ImGui::Text("IRQ Mode");
    ImGui::Text("IRQ Mode");

    ImGui::Text("Clock Source");

    ImGui::Text("IRQ allowed");
    ImGui::Text("Reached target");
    ImGui::Text("Reached max value");

    ImGui::NextColumn();

    for (u32 i = 0; i < 3; i++) {
        auto& timer = timers[i];
        ImGui::Text("TMR%u [%s]", i, timer->paused ? "paused" : "running");
        ImGui::Text("%u", timer->counter);
        ImGui::Text("%u [%s]", timer->target & 0xFFFF,
                    timer->mode.reset_mode == ResetMode::AfterTarget ? "enabled" : "disabled");
        ImGui::Text("%s", timer->mode.sync_enabled ? "enabled" : "disabled");
        ImGui::TextColored(timer->mode.sync_enabled ? white : grey, "%u", timer->mode.sync_mode.GetValue());
        ImGui::Text("%s", timer->mode.irq_on_target ? "yes" : "no");
        ImGui::Text("%s", timer->mode.irq_on_max_value ? "yes" : "no");
        ImGui::Text("%s", timer->mode.irq_repeat_mode ? "Repeat" : "One-Shot");
        ImGui::Text("%s", timer->mode.irq_toggle_mode ? "Toggle" : "Pulse");
        ImGui::Text("%s", ClockSourceName(timer));
        ImGui::Text("%s", timer->mode.allow_irq ? "yes" : "no");
        ImGui::Text("%s", timer->mode.reached_target ? "true" : "false");
        ImGui::Text("%s", timer->mode.reached_max_value ? "true" : "false");
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::End();
}
