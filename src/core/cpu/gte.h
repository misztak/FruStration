#pragma once

#include "types.h"

class GTE {
public:
    void Reset();

    void SetReg(u32 index, u32 value);

private:

    struct Vec3 {
        s16 x = 0, y = 0, z = 0;
    };
};