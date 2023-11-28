#pragma once

#include <array>

#include "util/types.h"
#include "util/type_util.h"

template<typename ValueType>
struct Vector3 {
    void SetXYFromU32(u32 value);
    u32 GetXYAsU32();

    union {
        struct { ValueType x, y, z; };
        struct { ValueType r, g, b; };
        struct { ValueType ir1, ir2, ir3; };
        struct { ValueType mac1, mac2, mac3; };
    };
};

#define Elem1D(x) elems[(x) / rows][(x) % rows]

template<u32 rows, u32 columns>
struct Matrix {
    using Column = std::array<s16, columns>;
    using Elements = std::array<Column, rows>;

    Elements elems = {};

    void SetPairFromOffset(u32 value, u32 offset) {
        Elem1D(offset) = static_cast<s16>(value);
        Elem1D(offset + 1) = static_cast<s16>(value >> 16);
    }

    u32 GetPairFromOffset(u32 offset) {
        u32 lsbs = ZeroExtend32(Elem1D(offset));
        u32 msbs = ZeroExtend32(Elem1D(offset + 1));
        return (msbs << 16) | (lsbs & 0xFFFF);
    }

    Column& operator[](usize i) {
        return elems[i];
    }
};

#undef Elem1D

using Matrix3x3 = Matrix<3, 3>;
