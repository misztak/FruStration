#include "system.h"

#include "bus.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"
#include "interrupt.h"
#include "timer.h"
#include "debugger.h"
#include "macros.h"

LOG_CHANNEL(System);

System::System() = default;

System::~System() = default;

void System::Init() {
    cpu = std::make_unique<CPU::CPU>();
    bus = std::make_unique<BUS>();
    dma = std::make_unique<DMA>();
    gpu = std::make_unique<GPU>();
    interrupt = std::make_unique<InterruptController>();
    timers = std::make_unique<TimerController>();
    debugger = std::make_unique<Debugger>();

    cpu->Init(bus.get(), debugger.get());
    bus->Init(dma.get(), gpu.get(), cpu.get(), interrupt.get(), timers.get(), debugger.get());
    dma->Init(bus.get(), gpu.get(), interrupt.get());
    gpu->Init();
    interrupt->Init(cpu.get());
    debugger->Init(cpu.get());
    LOG_INFO << "Initialized PSX core";
}

bool System::LoadBIOS(const std::string& bios_path) {
    return bus->LoadBIOS(bios_path);
}

void System::RunFrame() {
    cpu->Step();
}

void System::Run() {
    while (true) {
        cpu->Step();
    }
}

void System::Reset() {
    cpu->Reset();
    bus->Reset();
    dma->Reset();
    gpu->Reset();
    interrupt->Reset();
    timers->Reset();
    debugger->Reset();
    LOG_INFO << "System reset";
}

void System::DrawDebugWindows() {
    if (draw_mem_viewer) bus->DrawMemEditor(&draw_mem_viewer);
    if (draw_cpu_state) cpu->DrawCpuState(&draw_cpu_state);
    if (draw_gpu_state) gpu->DrawGpuState(&draw_gpu_state);
    if (draw_debugger) debugger->DrawDebugger(&draw_debugger);
}

u16* System::GetVRAM() {
    return gpu->GetVRAM();
}

bool System::IsHalted() {
    return cpu->halt;
}

void System::SetHalt(bool halt) {
    cpu->halt = halt;
    LOG_INFO << "System " << (halt ? "paused" : "resumed");
}

void System::VBlank() {
    interrupt->Request(IRQ::VBLANK);
}
