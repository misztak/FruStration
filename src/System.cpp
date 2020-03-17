#include "System.hpp"

#include "BUS.hpp"
#include "CPU.hpp"

System::System() = default;

System::~System() = default;

void System::Init() {
    cpu = std::make_unique<CPU>();
    bus = std::make_unique<BUS>();

    cpu->Init(bus.get());
}

bool System::LoadBIOS(const std::string& bios_path) {
    return bus->LoadBIOS(bios_path);
}

void System::Run() {
    while (true) {
        cpu->Step();
    }
}
