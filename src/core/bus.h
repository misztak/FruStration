#pragma once

#include <string>
#include <vector>

#include "types.h"

class DMA;
class GPU;
class CDROM;
class InterruptController;
class TimerController;
class Debugger;

namespace CPU {
class CPU;
}

class BUS {
public:
    BUS();
    void Init(DMA* dma, GPU* gpu, CPU::CPU* cpu, CDROM* cdrom, InterruptController* interrupt, TimerController* timers, Debugger* debugger);
    void Reset();
    bool LoadBIOS(const std::string& path);
    bool LoadPsExe(const std::string& path);

    void DrawMemEditor(bool* open);
    void DumpRAM(const std::string& path);

    template <typename ValueType> ValueType Load(u32 address);
    template <typename Value> void Store(u32 address, Value value);

    u8 Read(u32 address);
private:
    ALWAYS_INLINE u32 MaskRegion(u32 address) { return address & MEM_REGION_MASKS[address >> 29]; }

    static constexpr u32 BIOS_SIZE = 512 * 1024;
    static constexpr u32 RAM_SIZE = 2048 * 1024;
    static constexpr u32 SCRATCH_SIZE = 1024;
    static constexpr u32 CACHE_CTRL_SIZE = 512;
    static constexpr u32 IO_PORTS_SIZE = 8 * 1024;
    static constexpr u32 EXP_REG_1_SIZE = 8192 * 1024;
    static constexpr u32 EXP_REG_2_SIZE = 8 * 1024;
    static constexpr u32 EXP_REG_3_SIZE = 2048 * 1024;

    static constexpr u32 BIOS_START = 0x1FC00000;
    static constexpr u32 RAM_START = 0x00000000;
    static constexpr u32 SCRATCH_START = 0x1F800000;
    static constexpr u32 CACHE_CTRL_START = 0xFFFE0000;
    static constexpr u32 IO_PORTS_START = 0x1F801000;
    static constexpr u32 EXP_REG_1_START = 0x1F000000;
    static constexpr u32 EXP_REG_2_START = 0x1F802000;
    static constexpr u32 EXP_REG_3_START = 0x1FA00000;

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
    CDROM* cdrom = nullptr;
    InterruptController* interrupt = nullptr;
    TimerController* timers = nullptr;
    Debugger* debugger = nullptr;

    std::vector<u8> bios;
    std::vector<u8> ram;
    std::vector<u8> scratchpad;
};
