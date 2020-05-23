#pragma once

#include <memory>
#include <string>

#include "types.h"

namespace CPU {
class CPU;
}

class BUS;
class DMA;
class GPU;

class System {
public:
    System();
    ~System();
    void Init();
    bool LoadBIOS(const std::string& bios_path);
    void RunFrame();
    void Run();

    // TODO: maybe make Display a friend class?
    u16* GetVRAM();
private:
    std::unique_ptr<CPU::CPU> cpu;
    std::unique_ptr<BUS> bus;
    std::unique_ptr<DMA> dma;
    std::unique_ptr<GPU> gpu;
};
