#include "system.h"

#include "bus.h"
#include "cpu.h"
#include "dma.h"

System::System() = default;

System::~System() = default;

void System::Init() {
    cpu = std::make_unique<CPU::CPU>();
    bus = std::make_unique<BUS>();
    dma = std::make_unique<DMA>();

    cpu->Init(bus.get());
    bus->Init(dma.get());
    dma->Init(bus.get());
}

bool System::LoadBIOS(const std::string& bios_path) {
    return bus->LoadBIOS(bios_path);
}

void System::Run() {
    while (true) {
        cpu->Step();
    }
}
