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
    xori = 0x0E,
    lui = 0x0F,
    cop0 = 0x10,
    cop1 = 0x11,
    cop2 = 0x12,
    cop3 = 0x13,
    lb = 0x20,
    lh = 0x21,
    lwl = 0x22,
    lw = 0x23,
    lbu = 0x24,
    lhu = 0x25,
    lwr = 0x26,
    sb = 0x28,
    sh = 0x29,
    swl = 0x2A,
    sw = 0x2B,
    swr = 0x2E,
    lwc0 = 0x30,
    lwc1 = 0x31,
    lwc2 = 0x32,
    lwc3 = 0x33,
    swc0 = 0x38,
    swc1 = 0x39,
    swc2 = 0x3A,
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
    breakpoint = 0x0D,
    mfhi = 0x10,
    mthi = 0x11,
    mflo = 0x12,
    mtlo = 0x13,
    mult = 0x18,
    multu = 0x19,
    div = 0x1A,
    divu = 0x1B,
    add = 0x20,
    addu = 0x21,
    sub = 0x22,
    subu = 0x23,
    andr = 0x24,
    orr = 0x25,
    xorr = 0x26,
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
    Interrupt = 0x00,
    LoadAddress = 0x04,
    StoreAddress = 0x05,
    Syscall = 0x08,
    Break = 0x09,
    ReservedInstr = 0x0A,
    CopError = 0x0B,
    Overflow = 0x0C,
};

constexpr u32 GP_REG_COUNT = 32;
union GP_Registers {
    u32 r[GP_REG_COUNT] = {};

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

constexpr u32 SP_REG_COUNT = 3;
union SP_Registers {
    u32 spr[SP_REG_COUNT] = {};

    struct {
        u32 pc;  // we set the pc to 0xbfc00000 during cpu init
        u32 hi;
        u32 lo;
    };
};

constexpr u32 COP_REG_COUNT = 16;
union CP0_Registers {
    u32 cpr[COP_REG_COUNT] = {};

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
        // system status register (R/W)
        union {
            u32 value;
            BitField<u32, bool, 0, 1> interrupt_enable;
            BitField<u32, u32, 8, 8> IM;
            BitField<u32, bool, 16, 1> isolate_cache;
        } sr;
        // describes the most recently recognised exception (R)
        union {
            u32 value;
            BitField<u32, u32, 2, 5> excode;
            BitField<u32, u32, 8, 8> IP;
            BitField<u32, u32, 28, 1> CE;
            BitField<u32, u32, 31, 1> BD;
        } cause;
        u32 epc;        // return address from trap (R)
        u32 prid;       // processor ID (R)
    };
};

const char* const rnames[GP_REG_COUNT] = {
    "0", "at", "v0", "v1",
    "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3",
    "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3",
    "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1",
    "gp", "sp", "fp", "ra"
};

const char* const spnames[SP_REG_COUNT] = {
    "pc", "hi", "lo"
};

const char* const coprnames[COP_REG_COUNT] = {
    "0", "1", "2", "bpc",
    "4", "bda", "jumpdest", "dcic",
    "bad_vaddr", "bdam", "10", "bpcm",
    "sr", "cause", "epc", "prid"
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
