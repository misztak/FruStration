#include "system.h"

#include "bus.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"
#include "nano_log.h"

LOG_CHANNEL(System);

System::System() = default;

System::~System() = default;

void System::Init() {
    cpu = std::make_unique<CPU::CPU>();
    bus = std::make_unique<BUS>();
    dma = std::make_unique<DMA>();
    gpu = std::make_unique<GPU>();

    cpu->Init(bus.get());
    bus->Init(dma.get(), gpu.get(), cpu.get());
    dma->Init(bus.get(), gpu.get());
    gpu->Init();
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
    LOG_INFO << "System reset";
}

void System::DrawDebugWindows() {
    if (draw_mem_viewer) bus->DrawMemEditor(&draw_mem_viewer);
    if (draw_cpu_state) cpu->DrawCpuState(&draw_cpu_state);
    if (draw_gpu_state) gpu->DrawGpuState(&draw_gpu_state);
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
