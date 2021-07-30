#pragma once

#include "types.h"
#include "bitfield.h"

class CDROM {
public:
    CDROM();
    void Init();
    void Reset();
    void Step();
    u8 Load(u32 address);
    void Store(u32 address, u8 value);

private:
    enum class ParFifo: u8 {
        EMPTY = 1, FULL = 0
    };
    enum class ResFifo: u8 { EMPTY };
    enum class DatFifo: u8 { EMPTY };

    union {
        BitField<u8, u8, 0, 2> index;
        BitField<u8, bool, 2, 1> adp_busy;
        BitField<u8, ParFifo, 3, 1> par_empty;
        BitField<u8, ParFifo, 4, 1> par_full;
        BitField<u8, ResFifo, 5, 1> res_empty;
        BitField<u8, DatFifo, 6, 1> dat_empty;
        BitField<u8, bool, 7, 1> busy;
        u8 value = 0;
    } status;

    u8 command = 0;
    // parameter fifo
    u8 request = 0;

    // data fifo
    u8 response_fifo[16] = {};

    u8 interrupt_enable = 0;
    u8 interrupt_flag = 0;

};