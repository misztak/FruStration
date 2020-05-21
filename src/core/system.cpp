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
    while (!gpu->frame_done_hack && !cpu->halt) {
        cpu->Step();
    }
    gpu->frame_done_hack = false;
    gpu->Draw();
}

void System::Run() {
    while (true) {
        cpu->Step();
    }
}
