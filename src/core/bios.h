#pragma once

#include <string>

#include "util/types.h"

class System;

class BIOS {
public:
    explicit BIOS(System* system);
    void TraceFunction(u32 address, u32 index);

    void PutChar(s8 value);

private:
    static constexpr u32 A_FUNCTION_COUNT = 181;
    static constexpr u32 B_FUNCTION_COUNT = 94;
    static constexpr u32 C_FUNCTION_COUNT = 30;

    void TraceAFunction(u32 index);
    void TraceBFunction(u32 index);
    void TraceCFunction(u32 index);

    std::string stdout_buffer;

    System* sys = nullptr;
};
