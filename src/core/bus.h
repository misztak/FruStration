#pragma once

#include <string>
#include <vector>

#include "types.h"

class DMA;
class GPU;
class InterruptController;

namespace CPU {
class CPU;
}

class BUS {
public:
    BUS();
    void Init(DMA* dma, GPU* gpu, CPU::CPU* cpu, InterruptController* interrupt);
    void Reset();
    bool LoadBIOS(const std::string& path);
    bool LoadPsExe(const std::string& path);

    void DrawMemEditor(bool* open);
    void DumpRAM(const std::string& path);

    template <typename ValueType> ValueType Load(u32 address);
    template <typename Value> void Store(u32 address, Value value);
private:
    ALWAYS_INLINE u32 MaskRegion(u32 address) { return address & MEM_REGION_MASKS[address >> 29]; }

    static constexpr u32 BIOS_FILE_SIZE = 512 * 1024;
    static constexpr u32 RAM_SIZE = 2048 * 1024;
    static constexpr u32 SCRATCH_SIZE = 1024;

    const u32 MEM_REGION_MASKS[8] = {
        // KUSEG - 2048 MB
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        // KSEG0 - 512 MB
        0x7FFFFFFF,
        // KSEG1 - 512 MB
        0x1FFFFFFF,
        // KSEG2 - 1024 MB
        0xFFFFFFFF, 0xFFFFFFFF,
    };

    DMA* dma = nullptr;
    GPU* gpu = nullptr;
    CPU::CPU* cpu = nullptr;
    InterruptController* interrupt = nullptr;

    std::vector<u8> bios;
    std::vector<u8> ram;
    std::vector<u8> scratchpad;
};
