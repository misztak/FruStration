#include "timer.h"

#include "gpu.h"
#include "interrupt.h"
#include "imgui.h"
#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(Timer);

TimerController::TimerController() {}

void TimerController::Init(GPU* g, InterruptController* icontroller) {
    gpu = g;
    interrupt_controller = icontroller;
}

void TimerController::Step(u32 steps, u32 timer_index) {
    auto& timer = timers[timer_index];

    bool sync = timer.mode.sync_enabled;

    if (sync && timer_index == TMR0) {
        switch (timer.mode.sync_mode) {
            case 0:
                timer.paused = gpu->in_hblank;
                break;
            case 1:
                if (gpu->in_hblank) timer.counter = 0;
                break;
            case 2:
                if (gpu->in_hblank) timer.counter = 0;
                timer.paused = !gpu->in_hblank;
                break;
            case 3:
                if (timer.paused && gpu->in_hblank) {
                    timer.paused = false;
                    timer.mode.sync_enabled = false;
                }
                break;
        }
    } else if (sync && timer_index == TMR1) {
        switch (timer.mode.sync_mode) {
            case 0:
                timer.paused = gpu->in_vblank;
                break;
            case 1:
                if (gpu->in_vblank) timer.counter = 0;
                break;
            case 2:
                if (gpu->in_vblank) timer.counter = 0;
                timer.paused = !gpu->in_vblank;
                break;
            case 3:
                if (timer.paused && gpu->in_vblank) {
                    timer.paused = false;
                    timer.mode.sync_enabled = false;
                }
                break;
        }
    } else if (sync && timer_index == TMR2) {
        switch (timer.mode.sync_mode) {
            case 0:
            case 3:
                timer.paused = true;
                break;
            case 1:
            case 2:
                break;
        }
    }

    if (timer.paused) return;

    Increment(timer_index, steps);
    bool send_irq = false;

    if (timer.mode.reset_mode == ResetMode::AfterTarget && (timer.counter >= (timer.target & 0xFFFF))) {
        timer.counter = 0;
        timer.mode.reached_target = true;
        if (timer.mode.irq_on_target) send_irq = true;
    }

    if (timer.mode.reset_mode == ResetMode::AfterMaxValue && (timer.counter >= 0xFFFF)) {
        timer.counter = 0;
        timer.mode.reached_max_value = true;
        if (timer.mode.irq_on_max_value) send_irq = true;
    }

    if (send_irq && timer.mode.allow_irq) {
        // one-shot mode
        if (!timer.mode.irq_repeat_mode)
            timer.mode.allow_irq = false;

        // TODO: in pulse mode Bit10 is zero for a couple of clock cycles while an interrupt occurs

        SendIRQ(timer_index);
    }

    if (send_irq && timer.mode.irq_toggle_mode && timer.mode.irq_repeat_mode)
        timer.mode.allow_irq = !timer.mode.allow_irq;

}

u32 TimerController::Load(u32 address) {
#if 0
    LOG_DEBUG << fmt::format("Load call to Timers [@ 0x{:02X}]", address);
#endif

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    if (timer_address == 0x0) {
        return timers[timer_index].counter;
    }

    if (timer_address == 0x4) {
        // reached flags get reset after read
        u32 value = timers[timer_index].mode.value;
        timers[timer_index].mode.reached_target = false;
        timers[timer_index].mode.reached_max_value = false;
        return value;
    }

    if (timer_address == 0x8) {
        return timers[timer_index].target;
    }

    Panic("Invalid timer register");
    return 0;
}

void TimerController::Store(u32 address, u32 value) {
    LOG_DEBUG << fmt::format("Store call to Timers [0x{:08X} @ 0x{:08X}]", value, address);

    const u32 timer_index = (address & 0xF0) >> 4;
    const u32 timer_address = address & 0xF;
    DebugAssert(timer_index <= 2);

    auto& timer = timers[timer_index];

    if (timer_address == 0x0) {
        timer.counter = value;
        return;
    }

    if (timer_address == 0x4) {
        // reset counter value
        timer.counter = 0;

        timer.mode.value = value;
        timer.mode.allow_irq = true;

        if (timer_index == 0 || timer_index == 1) {
            if (timer.mode.sync_mode == 3) timer.paused = true;
        } else {
            if (timer.mode.sync_mode == 0 || timer.mode.sync_mode == 3) timer.paused = true;
        }

        return;
    }

    if (timer_address == 0x8) {
        timer.target = value;
        return;
    }

    Panic("Invalid timer register");
}

void TimerController::SendIRQ(u32 index) {
    static constexpr IRQ timer_irq[3] = {
        IRQ::TIMER0,
        IRQ::TIMER1,
        IRQ::TIMER2,
    };

    DebugAssert(index < 3);
    interrupt_controller->Request(timer_irq[index]);
}

TimerController::ClockSource TimerController::GetClockSource(u32 index) {
    auto& timer = timers[index];

    switch (index) {
        case 0:
            if (timer.mode.clock_source == 0 || timer.mode.clock_source == 2)
                return ClockSource::SysClock;
            else
                return ClockSource::DotClock;
        case 1:
            if (timer.mode.clock_source == 0 || timer.mode.clock_source == 2)
                return ClockSource::SysClock;
            else
                return ClockSource::HBlank;
        case 2:
            if (timer.mode.clock_source == 0 || timer.mode.clock_source == 1)
                return ClockSource::SysClock;
            else
                return ClockSource::SysClockEighth;
        default:
            Panic("Invalid timer index");
    }
}

void TimerController::Increment(u32 index, u32 steps) {
    constexpr double CPU_CLOCK_RATE = 11.0 / 7.0;
    auto& timer = timers[index];

    timer.acc_steps += steps;

    ClockSource clock_source = GetClockSource(index);

    switch (clock_source) {
        case ClockSource::SysClock:
            timer.counter += static_cast<u32>(timer.acc_steps / CPU_CLOCK_RATE);
            timer.acc_steps = 0;
            break;
        case ClockSource::SysClockEighth:
            timer.counter += static_cast<u32>(timer.acc_steps / 8 / CPU_CLOCK_RATE);
            timer.acc_steps %= static_cast<u32>(8 * CPU_CLOCK_RATE);
            break;
        case ClockSource::HBlank:
            timer.counter += static_cast<u32>(timer.acc_steps / gpu->CyclesPerScanline());
            timer.acc_steps %= gpu->CyclesPerScanline();
            break;
        case ClockSource::DotClock:
            timer.counter += static_cast<u32>(timer.acc_steps / gpu->DotClock());
            timer.acc_steps %= gpu->DotClock();
            break;
    }
}

void TimerController::Reset() {
    for (auto& timer: timers) {
        timer.counter = 0;
        timer.mode.value = 0;
        timer.target = 0;
        timer.paused = false;
        timer.acc_steps = 0;
    }
}

void TimerController::DrawTimerState(bool* open) {
    ImGui::Begin("Timer State", open);
    ImGui::Columns(4);

    const ImVec4 white(1.0, 1.0, 1.0, 1.0);
    const ImVec4 grey(0.5, 0.5, 0.5, 1.0);
    const char* const clock_source[4] = {
        "System Clock", "System Clock / 8", "DotClock", "HBlank"
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
        ImGui::Text("%s", clock_source[static_cast<u32>(GetClockSource(i))]);
        ImGui::Text("%s", timer.mode.allow_irq ? "yes" : "no");
        ImGui::Text("%s", timer.mode.reached_target ? "true" : "false");
        ImGui::Text("%s", timer.mode.reached_max_value ? "true" : "false");
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::End();
}
