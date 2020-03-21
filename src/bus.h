#pragma once

#include <string>
#include <vector>

#include "types.h"

class BUS {
public:
    BUS();
    void Init();
    bool LoadBIOS(const std::string& path);

    u32 Load32(u32 address);
    void Store32(u32 address, u32 value);
private:
    ALWAYS_INLINE u32 MaskRegion(u32 address);

    enum : u32 { BIOS_FILE_SIZE = 512 * 1024 };
    static constexpr u32 RAM_SIZE = 2048 * 1024;

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

    std::vector<u8> bios;
    std::vector<u8> ram;
};
