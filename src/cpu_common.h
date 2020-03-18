#pragma once

#include "bitfield.h"
#include "types.h"

namespace CPU {

enum class PrimaryOpcode : u32 {
    special = 0x00,
    jmp = 0x02,
    addiu = 0x09,
    ori = 0x0D,
    lui = 0x0F,
    sw = 0x2B,
};

enum class SecondaryOpcode : u32 {
    sll = 0x00,
    sra = 0x03,
};

union GP_Registers {
    u32 r[32] = {};

    struct {
        u32 zero;  // always zero
        u32 at;    // assembler temporary
        u32 v0;    // function return values
        u32 v1;
        u32 a0;  // function arguments
        u32 a1;
        u32 a2;
        u32 a3;
        u32 t0;  // temporary registers
        u32 t1;
        u32 t2;
        u32 t3;
        u32 t4;
        u32 t5;
        u32 t6;
        u32 t7;
        u32 s0;  // saved registers
        u32 s1;
        u32 s2;
        u32 s3;
        u32 s4;
        u32 s5;
        u32 s6;
        u32 s7;
        u32 t8;  // more temporary registers
        u32 t9;
        u32 k0;  // kernel reserved registers
        u32 k1;
        u32 gp;  // global pointer
        u32 sp;  // stack pointer
        u32 fp;  // frame pointer
        u32 ra;  // function return address
    };
};

struct SP_Registers {
    u32 pc = 0xbfc00000;
    u32 hi = 0;
    u32 lo = 0;
};

union Instruction {
    u32 value = 0;

    union {
        BitField<u32, u32, 0, 16> imm;
        BitField<u32, u32, 16, 5> rt;
        BitField<u32, u32, 21, 5> rs;
        BitField<u32, PrimaryOpcode, 26, 6> op;
    } n;

    union {
        BitField<u32, SecondaryOpcode, 0, 6> sop;
        BitField<u32, u32, 6, 5> sa;
        BitField<u32, u32, 11, 5> rd;
        BitField<u32, u32, 16, 5> rt;
        BitField<u32, u32, 21, 5> rs;
        BitField<u32, u32, 26, 5> special;
    } s;

    BitField<u32, u32, 0, 26> jump_target;

    //struct {
    //    u32 imm : 16;
    //    u32 rt : 5;
    //    u32 rs : 5;
    //    PrimaryOpcode op : 6;
    //};

    ALWAYS_INLINE u32 imm_se() { return static_cast<s16>(n.imm); }
};

}  // namespace CPU
