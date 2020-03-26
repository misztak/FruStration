#pragma once

#include "bitfield.h"
#include "types.h"

namespace CPU {

enum class PrimaryOpcode : u32 {
    special = 0x00,
    bxxx = 0x01,
    jmp = 0x02,
    jal = 0x03,
    beq = 0x04,
    bne = 0x05,
    blez = 0x06,
    bgtz = 0x07,
    addi = 0x08,
    addiu = 0x09,
    slti = 0x0A,
    sltiu = 0x0B,
    andi = 0x0C,
    ori = 0x0D,
    lui = 0x0F,
    cop0 = 0x10,
    cop2 = 0x12,
    lb = 0x20,
    lh = 0x21,
    lw = 0x23,
    lbu = 0x24,
    lhu = 0x25,
    sb = 0x28,
    sh = 0x29,
    sw = 0x2B,
};

enum class SecondaryOpcode : u32 {
    sll = 0x00,
    srl = 0x02,
    sra = 0x03,
    sllv = 0x04,
    srlv = 0x06,
    srav = 0x07,
    jr = 0x08,
    jalr = 0x09,
    syscall = 0x0C,
    mfhi = 0x10,
    mthi = 0x11,
    mflo = 0x12,
    mtlo = 0x13,
    multu = 0x19,
    div = 0x1A,
    divu = 0x1B,
    add = 0x20,
    addu = 0x21,
    subu = 0x23,
    andd = 0x24,
    orr = 0x25,
    nor = 0x27,
    slt = 0x2A,
    sltu = 0x2B,
};

enum class CoprocessorOpcode : u32 {
    mf = 0x00,
    mt = 0x04,
    rfe = 0x10,
};

enum class ExceptionCode : u32 {
    LoadAddress = 0x04,
    StoreAddress = 0x05,
    Syscall = 0x08,
    Overflow = 0x0C,
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
    u32 pc = 0; // we set the pc to 0xbfc00000 during cpu init
    u32 hi = 0;
    u32 lo = 0;
};

union CP0_Registers {
    u32 cpr[16] = {};

    struct {
        u32 cop0r0;
        u32 cop0r1;
        u32 cop0r2;
        u32 bpc;        // breakpoint on execute (R/W)
        u32 cop0r4;
        u32 bda;        // breakpoint on data access (R/W)
        u32 jumpdest;   // randomly memorized jump address (R)
        u32 dcic;       // breakpoint control (R/W)
        u32 bad_vaddr;  // bad virtual address (R)
        u32 bdam;       // data access breakpoint mask (R/W)
        u32 cop0r10;
        u32 bpcm;       // execute breakpoint mask (R/W)
        u32 sr;         // system status register (R/W)
        u32 cause;      // describes the most recently recognised exception (R)
        u32 epc;        // return address from trap (R)
        u32 prid;       // processor ID (R)
    };
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

    union {
        BitField<u32, u32, 11, 5> rd;
        BitField<u32, u32, 16, 5> rt;
        BitField<u32, CoprocessorOpcode, 21, 5> cop_op;
        BitField<u32, u32, 26, 2> nr;
        BitField<u32, u32, 26, 6> op;
    } cop;

    ALWAYS_INLINE u32 imm_se() { return static_cast<s16>(n.imm); }
};

struct LoadDelayEntry {
    u32 reg = 0;
    u32 value = 0;
};

}  // namespace CPU
