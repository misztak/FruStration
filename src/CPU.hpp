#pragma once

#include "Types.hpp"

class BUS;

class CPU {
public:
    CPU();
    void Init(BUS* bus);

    void Step();
    u32 Load32(u32 address);
    void Store32(u32 address, u32 value);
private:
    u32 pc = 0xbfc00000;

    BUS* bus = nullptr;
};

