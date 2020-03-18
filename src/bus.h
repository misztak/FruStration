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
    enum : u32 { BIOS_FILE_SIZE = 512 * 1024 };

    std::vector<u8> bios;
};
