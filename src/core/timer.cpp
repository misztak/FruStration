#include "timer.h"

#include "gpu.h"
#include "interrupt.h"
#include "scheduler.h"
#include "imgui.h"
#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(Timer);

void TimerController::Timer::UpdateOnBlankFlip(u32 index, bool entered_blank) {
    // nothing to do if blank state didn't change
    if (gpu_currently_in_blank == entered_blank) return;

    gpu_currently_in_blank = entered_blank;

    if (gpu_currently_in_blank && mode.sync_enabled) {
        switch (mode.sync_mode) {
            case 0:
                break;
            case 1:
            case 2:
                counter = 0;
                break;
            case 3:
                mode.sync_enabled = false;
                break;
        }
    }

    UpdatePaused(index);
}

void TimerController::Timer::UpdatePaused(u32 index) {
    DebugAssert(index < 3);

    if (mode.sync_enabled) {
        if (index < 2) {
            switch (mode.sync_mode) {
                case 0: paused = gpu_currently_in_blank; break;
                case 1: paused = false; break;
                case 2: case 3: paused = !gpu_currently_in_blank; break;
            }
        } else {
            // stop counter 'forever' in syncmode 0 and 3
            // otherwise free run
            paused = paused && (mode.sync_mode % 3 == 0);
        }
    } else {
        // free run
        paused = false;
    }
}

u32 TimerController::Timer::CyclesUntilNextIRQ() {
    u32 cycles = std::numeric_limits<u32>::max();

    if (!paused) {
        if (mode.irq_on_target) {
            const u32 cycles_until_target = (counter < target) ? (target - counter) : ((MAX_COUNTER - counter) + target);
            cycles = std::min(cycles, cycles_until_target);
        }

        if (mode.irq_on_max_value) {
            const u32 cycles_until_max_value = MAX_COUNTER - counter;
            cycles = std::min(cycles, cycles_until_max_value);
        }
    }

    return cycles;
}

bool TimerController::Timer::Increment(u32 cycles) {
    if (paused) return false;

    DebugAssert(cycles <= CyclesUntilNextIRQ());

    const u32 previous_counter_value = counter;
    counter += cycles;

    bool send_irq = false;

    bool already_reached_target = previous_counter_value >= target;
    if (counter >= target && (!already_reached_target || target == 0)) {
        mode.reached_target = true;
        send_irq |= mode.irq_on_target;
        if (mode.reset_mode == ResetMode::AfterTarget && target > 0) {
            counter %= target;
        }
    }

    if (counter >= MAX_COUNTER) {
        mode.reached_max_value = true;
        send_irq |= mode.irq_on_max_value;
        counter %= MAX_COUNTER;
    }

    auto SendIRQ = [&] {
        if (!pending_irq || mode.irq_repeat_mode) {
            pending_irq = true;
            return true;
        }
        return false;
    };

    if (send_irq) {
        if (mode.irq_toggle_mode) {
            if (mode.irq_repeat_mode || mode.allow_irq) {
                // toggle mode
                mode.allow_irq = !mode.allow_irq;
                // high to low transition
                if (!mode.allow_irq) {
                    return SendIRQ();
                }
            }
        } else {
            // pulse mode, Bit10 always set
            mode.allow_irq = true;
            return SendIRQ();
        }
    }

    return false;
}

TimerController::TimerController() {
    Scheduler::AddComponent(
        Component::TIMER,
        [this](u32 cycles) { StepTmp(cycles); },
        [this] { return CyclesUntilNextEvent(); });
}

void TimerController::Init(GPU* g, InterruptController* icontroller) {
    gpu = g;
    interrupt_controller = icontroller;
}

void TimerController::StepTmp(u32 cycles) {
    // timer 0
    auto& dot_timer = timers[0];
    if (dot_timer.mode.clock_source % 2 == 0) {
        if (dot_timer.Increment(cycles)) {
            interrupt_controller->Request(IRQ::TIMER0);
        }
    }

    // timer 1
    auto& hblank_timer = timers[1];
    if (hblank_timer.mode.clock_source % 2 == 0) {
        if (hblank_timer.Increment(cycles)) {
            interrupt_controller->Request(IRQ::TIMER1);
        }
    }

    // timer 2
    auto& cpu_timer = timers[2];
    const bool uses_div_8 = cpu_timer.mode.clock_source & 0x2;
    u32 ticks;
    if (uses_div_8) {
        ticks = (cycles + div_8_remainder) / 8;
        div_8_remainder = (cycles + div_8_remainder) % 8;
    } else {
        ticks = cycles;
    }

    if (cpu_timer.Increment(ticks)) {
        interrupt_controller->Request(IRQ::TIMER2);
        if (cpu_timer.mode.sync_enabled && cpu_timer.mode.sync_mode % 3 == 0) {
            // stop permanently
            cpu_timer.counter =
                (cpu_timer.mode.irq_on_target && cpu_timer.mode.reached_target) ? cpu_timer.target : MAX_COUNTER;
            cpu_timer.paused = true;
        }
    }
}

u32 TimerController::CyclesUntilNextEvent() {
    u32 min_cycles = std::numeric_limits<u32>::max();

    auto& dot_timer = timers[0];
    if (dot_timer.mode.clock_source % 2 == 0) {
        min_cycles = std::min(min_cycles, dot_timer.CyclesUntilNextIRQ());
    }

    auto& hblank_timer = timers[1];
    if (hblank_timer.mode.clock_source % 2 == 0) {
        min_cycles = std::min(min_cycles, hblank_timer.CyclesUntilNextIRQ());
    }

    auto& cpu_timer = timers[2];
    const u32 raw_cycles = cpu_timer.CyclesUntilNextIRQ();
    if (cpu_timer.mode.clock_source & 0x2) {
        if (raw_cycles != std::numeric_limits<u32>::max()) {
            min_cycles = std::min(min_cycles, raw_cycles * 8 - div_8_remainder);
        }
    } else {
        min_cycles = std::min(min_cycles, raw_cycles);
    }

    return min_cycles;
}

u32 TimerController::Load(u32 address) {
#if 0
    LOG_DEBUG << fmt::format("Load call to Timers [@ 0x{:02X}]", address);
#endif

    Scheduler::ForceUpdate();

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    u32 value;

    if (timer_address == 0x0) {
        value = timers[timer_index].counter;
    }

    else if (timer_address == 0x4) {
        // reached flags get reset after read
        value = timers[timer_index].mode.value;
        timers[timer_index].mode.reached_target = false;
        timers[timer_index].mode.reached_max_value = false;
    }

    else if (timer_address == 0x8) {
        value = timers[timer_index].target;
    }

    else {
        Panic("Invalid timer register");
    }

    Scheduler::RecalculateNextEvent();

    return value;
}

void TimerController::Store(u32 address, u32 value) {
    LOG_DEBUG << fmt::format("Store call to Timers [0x{:08X} @ 0x{:08X}]", value, address);

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    Scheduler::ForceUpdate();

    auto& timer = timers[timer_index];

    if (timer_address == 0x0) {
        timer.counter = value;
    }

    else if (timer_address == 0x4) {
        // reset IRQ
        timer.pending_irq = false;

        // reset counter value
        timer.counter = 0;

        timer.mode.value = (timer.mode.value & ~0b1110001111111111) | (value & 0b1110001111111111);

        if (timer.mode.irq_toggle_mode) {
            timer.mode.allow_irq = true;
        }

        timer.UpdatePaused(timer_index);
    }

    else if (timer_address == 0x8) {
        timer.target = value;
    }

    else {
        Panic("Invalid timer register");
    }

    Scheduler::RecalculateNextEvent();
}

u32 TimerController::Peek(u32 address) {
    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    if (timer_address == 0x0) {
        return timers[timer_index].counter;
    }
    if (timer_address == 0x4) {
        return timers[timer_index].mode.value;
    }
    if (timer_address == 0x8) {
        return timers[timer_index].target;
    }
    return 0;
}

void TimerController::Reset() {
    for (auto& timer: timers) {
        timer.counter = 0;
        timer.mode.value = 0;
        timer.target = 0;
        timer.paused = false;
        timer.pending_irq = false;
        timer.gpu_currently_in_blank = false;
    }

    div_8_remainder = 0;
}

void TimerController::DrawTimerState(bool* open) {
    ImGui::Begin("Timer State", open);
    ImGui::Columns(4);

    const ImVec4 white(1.0, 1.0, 1.0, 1.0);
    const ImVec4 grey(0.5, 0.5, 0.5, 1.0);

    const auto ClockSourceName = [](u32 timer_index, u32 source) {
        if (source % 2 == 0) return "System Clock";
        switch (timer_index) {
            case 0: return "DotClock";
            case 1: return "HBlank";
            case 2: return "System Clock / 8";
            default: return "XXX";
        }
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
        ImGui::Text("TMR%u [%s]", i, timer.paused ? "paused" : "running");
        ImGui::Text("%u", timer.counter);
        ImGui::Text("%u [%s]", timer.target & 0xFFFF, timer.mode.reset_mode == ResetMode::AfterTarget ? "enabled" : "disabled");
        ImGui::Text("%s", timer.mode.sync_enabled ? "enabled" : "disabled");
        ImGui::TextColored(timer.mode.sync_enabled ? white : grey, "%u", timer.mode.sync_mode.GetValue());
        ImGui::Text("%s", timer.mode.irq_on_target ? "yes" : "no");
        ImGui::Text("%s", timer.mode.irq_on_max_value ? "yes" : "no");
        ImGui::Text("%s", timer.mode.irq_repeat_mode ? "Repeat" : "One-Shot");
        ImGui::Text("%s", timer.mode.irq_toggle_mode ? "Toggle" : "Pulse");
        ImGui::Text("%s", ClockSourceName(i, timer.mode.clock_source));
        ImGui::Text("%s", timer.mode.allow_irq ? "yes" : "no");
        ImGui::Text("%s", timer.mode.reached_target ? "true" : "false");
        ImGui::Text("%s", timer.mode.reached_max_value ? "true" : "false");
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::End();
}
