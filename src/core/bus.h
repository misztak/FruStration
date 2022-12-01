#pragma once

#include <string>
#include <vector>

#include "types.h"

class System;

namespace CPU {
class CPU;
}

class BUS {
public:
    BUS(System* system);
    void Reset();
    bool LoadBIOS(const std::string& path);
    bool LoadPsExe(const std::string& path);

    void DrawMemEditor(bool* open);

    template<typename ValueType>
    ValueType Load(u32 address);
    template<typename Value>
    void Store(u32 address, Value value);

    // read from memory without causing side effects (i.e. MMIO state changes)
    u8 Peek(u32 address);
    u32 Peek32(u32 address);

private:
    ALWAYS_INLINE static u32 MaskRegion(u32 address) { return address & MEM_REGION_MASKS[address >> 29]; }

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

    static constexpr u32 PSEXE_HEADER_SIZE = 0x800;

    static constexpr u32 MEM_REGION_MASKS[8] = {
        // KUSEG - 2048 MB
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        // KSEG0 - 512 MB
        0x7FFFFFFF,
        // KSEG1 - 512 MB
        0x1FFFFFFF,
        // KSEG2 - 1024 MB
        0xFFFFFFFF,
        0xFFFFFFFF,
    };

    System* sys = nullptr;

    std::vector<u8> bios;
    std::vector<u8> ram;
    std::vector<u8> scratchpad;
};
