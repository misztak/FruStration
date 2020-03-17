#pragma once

namespace CPU {

enum class PrimaryOpcode : u8 {
    ori = 0x0D,
    lui = 0x0F,
};

struct GP_Registers {
    union {
        u32 r[32] = {};

        struct {
            u32 zero;   // always zero
            u32 at;     // assembler temporary
            u32 v0;     // function return values
            u32 v1;
            u32 a0;     // function arguments
            u32 a1;
            u32 a2;
            u32 a3;
            u32 t0;     // temporary registers
            u32 t1;
            u32 t2;
            u32 t3;
            u32 t4;
            u32 t5;
            u32 t6;
            u32 t7;
            u32 s0;     // saved registers
            u32 s1;
            u32 s2;
            u32 s3;
            u32 s4;
            u32 s5;
            u32 s6;
            u32 s7;
            u32 t8;     // more temporary registers
            u32 t9;
            u32 k0;     // kernel reserved registers
            u32 k1;
            u32 gp;     // global pointer
            u32 sp;     // stack pointer
            u32 fp;     // frame pointer
            u32 ra;     // function return address
        };
    };
};

struct SP_Registers {
    u32 pc = 0xbfc00000;
    u32 hi = 0;
    u32 lo = 0;
};

}
