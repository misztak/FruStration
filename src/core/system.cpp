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

LOG_CHANNEL(Emulator);

System::System() {
    cpu = std::make_unique<CPU::CPU>();
    bus = std::make_unique<BUS>();
    dma = std::make_unique<DMA>();
    gpu = std::make_unique<GPU>();
    cdrom = std::make_unique<CDROM>();
    interrupt = std::make_unique<InterruptController>();
    timers = std::make_unique<TimerController>();
    debugger = std::make_unique<Debugger>();

    LOG_INFO << "Initialized PSX core";
}

bool Emulator::LoadBIOS(const std::string& bios_path) {
    return sys.bus->LoadBIOS(bios_path);
}

void Emulator::Tick() {
    sys.cpu->Step();
}

bool Emulator::DrawNextFrame() {
    return sys.gpu->draw_frame;
}

void Emulator::ResetDrawFrame() {
    sys.gpu->draw_frame = false;
}

void Emulator::Reset() {
    sys.cpu->Reset();
    sys.bus->Reset();
    sys.dma->Reset();
    sys.gpu->Reset();
    sys.cdrom->Reset();
    sys.interrupt->Reset();
    sys.timers->Reset();
    sys.debugger->Reset();

    Scheduler::Reset();
    Scheduler::RecalculateNextEvent();

    LOG_INFO << "System reset";
}

bool Emulator::In24BPPMode() {
    return (sys.gpu->ReadStat() & (1u << 21)) != 0;
}

bool Emulator::IsHalted() {
    return sys.cpu->halt;
}

void Emulator::SetHalt(bool halt) {
    sys.cpu->halt = halt;
    LOG_INFO << "System " << (halt ? "paused" : "resumed");
}

u8* Emulator::GetVideoOutput() {
    return sys.gpu->GetVideoOutput();
}

u16* Emulator::GetVRAM() {
    return sys.gpu->GetVRAM();
}

void Emulator::StartGDBServer() {
    if (!cfg_gdb_server_enabled) return;

    SetHalt(true);

    Assert(sys.debugger.get());
    GDB::Init(42069, sys.debugger.get());
}

void Emulator::HandleGDBClientRequest() {
    if (!cfg_gdb_server_enabled) return;

    GDB::HandleClientRequest();
}

void Emulator::DrawDebugWindows() {
    if (draw_mem_viewer) sys.bus->DrawMemEditor(&draw_mem_viewer);
    if (draw_cpu_state) sys.cpu->DrawCpuState(&draw_cpu_state);
    if (draw_gpu_state) sys.gpu->DrawGpuState(&draw_gpu_state);
    if (draw_debugger) sys.debugger->DrawDebugger(&draw_debugger);
    if (draw_timer_state) sys.timers->DrawTimerState(&draw_timer_state);
}
