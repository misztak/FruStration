#include "system.h"

#include "bus.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"

System::System() = default;

System::~System() = default;

void System::Init() {
    cpu = std::make_unique<CPU::CPU>();
    bus = std::make_unique<BUS>();
    dma = std::make_unique<DMA>();
    gpu = std::make_unique<GPU>();

    cpu->Init(bus.get());
    bus->Init(dma.get(), gpu.get());
    dma->Init(bus.get(), gpu.get());
    gpu->Init();
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

u16* System::GetVRAM() {
    return gpu->GetVRAM();
}

bool System::IsHalted() {
    return cpu->halt;
}

void System::SetHalt(bool halt) {
    cpu->halt = halt;
    printf("System %s\n", halt ? "paused" : "resumed");
}
