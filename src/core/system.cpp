#include "system.h"

#include "bus.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"
#include "cdrom.h"
#include "interrupt.h"
#include "timer.h"
#include "scheduler.h"
#include "debugger.h"
#include "gdb_stub.h"
#include "macros.h"

LOG_CHANNEL(System);

System::System() = default;

System::~System() {
    GDB::Shutdown();
}

void System::Init() {
    cpu = std::make_unique<CPU::CPU>();
    bus = std::make_unique<BUS>();
    dma = std::make_unique<DMA>();
    gpu = std::make_unique<GPU>();
    cdrom = std::make_unique<CDROM>();
    interrupt = std::make_unique<InterruptController>();
    timers = std::make_unique<TimerController>();
    debugger = std::make_unique<Debugger>();

    cpu->Init(bus.get(), debugger.get());
    bus->Init(dma.get(), gpu.get(), cpu.get(), cdrom.get(), interrupt.get(), timers.get(), debugger.get());
    dma->Init(bus.get(), gpu.get(), interrupt.get());
    gpu->Init(timers.get(), interrupt.get());
    timers->Init(gpu.get(), interrupt.get());
    interrupt->Init(cpu.get());
    cdrom->Init(interrupt.get());
    debugger->Init(cpu.get(), bus.get());

    LOG_INFO << "Initialized PSX core";
}

bool System::LoadBIOS(const std::string& bios_path) {
    return bus->LoadBIOS(bios_path);
}

void System::Tick() {
    cpu->Step();
}

bool System::DrawNextFrame() {
    return gpu->draw_frame;
}

void System::ResetDrawFrame() {
    gpu->draw_frame = false;
}

void System::Step() {
    Panic("Fuck");

    // the step counter is static to prevent going out of sync with
    // the other components in case a breakpoint is hit and Step() exits early
    static u32 step = 0;

    // 100 cpu instructions - 2 cycles per instruction
    while (step < 100) {
        cpu->Step();
        step++;
        if (unlikely(cpu->halt)) return;
    }
    step = 0;

    // dma step
    cdrom->Step();

    timers->Step(300, TMR0);
    timers->Step(300, TMR1);
    timers->Step(300, TMR2);

    // video clock is cpu clock multiplied by (11 / 7)
    // also account for the cpu needing 2 cycles to finish one instruction
    gpu->Step(300);
}

void System::SingleStep() {
    Panic("Fuck");

    cpu->Step();

    // dma step
    cdrom->Step();

    timers->Step(3, TMR0);
    timers->Step(3, TMR1);
    timers->Step(3, TMR2);

    gpu->Step(3);
}

void System::VBlankCallback(std::function<void()> callback) {
    gpu->vblank_cb = nullptr;
}

void System::Reset() {
    cpu->Reset();
    bus->Reset();
    dma->Reset();
    gpu->Reset();
    cdrom->Reset();
    interrupt->Reset();
    timers->Reset();
    debugger->Reset();
    Scheduler::Reset();
    LOG_INFO << "System reset";
}

bool System::In24BPPMode() {
    return (gpu->ReadStat() & (1u << 21)) != 0;
}

bool System::IsHalted() {
    return cpu->halt;
}

void System::SetHalt(bool halt) {
    cpu->halt = halt;
    LOG_INFO << "System " << (halt ? "paused" : "resumed");
}

u8* System::GetVideoOutput() {
    return gpu->GetVideoOutput();
}

u16* System::GetVRAM() {
    return gpu->GetVRAM();
}

void System::StartGDBServer() {
    if (!cfg_gdb_server_enabled) return;

    SetHalt(true);

    Assert(debugger.get());
    GDB::Init(42069, debugger.get());
}

void System::HandleGDBClientRequest() {
    if (!cfg_gdb_server_enabled) return;

    GDB::HandleClientRequest();
}

void System::DrawDebugWindows() {
    if (draw_mem_viewer) bus->DrawMemEditor(&draw_mem_viewer);
    if (draw_cpu_state) cpu->DrawCpuState(&draw_cpu_state);
    if (draw_gpu_state) gpu->DrawGpuState(&draw_gpu_state);
    if (draw_debugger) debugger->DrawDebugger(&draw_debugger);
    if (draw_timer_state) timers->DrawTimerState(&draw_timer_state);
}
