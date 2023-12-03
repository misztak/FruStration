#pragma once

#include "util/types.h"
#include "util/bitfield.h"
#include "controller.h"

class System;

// A peripheral can be either a controller or memory card
class Peripherals {
public:
    explicit Peripherals(System* sys);

    u32 Read(u32 address);
    void Write(u32 address, u32 value);

    Controller& GetController1();
private:
    static constexpr u16 MODE_MASK = 0x013F;
    static constexpr u16 CTRL_MASK = 0x0078;

    union {
        BitField<u32, bool, 0, 1> tx_ready_1;
        BitField<u32, bool, 1, 1> rx_fifo_not_empty;
        BitField<u32, bool, 2, 1> tx_ready_2;
        BitField<u32, bool, 3, 1> rx_parity_error;
        BitField<u32, u32, 7, 1> ack_in_level;
        BitField<u32, bool, 9, 1> irq;
        BitField<u32, u32, 11, 21> baudrate_timer;
        u32 bits = 0;
    } stat;

    union {
        BitField<u16, u16, 0, 2> baud_mul_factor;
        BitField<u16, u16, 2, 2> char_length;
        BitField<u16, bool, 4, 1> parity_enable;
        BitField<u16, u16, 5, 1> parity_type;
        BitField<u16, u16, 8, 1> clk_output_polarity;
        u16 bits = 0;
    } mode;

    union {
        BitField<u16, bool, 0, 1> tx_enable;
        BitField<u16, u16, 1, 1> joy_output;
        BitField<u16, bool, 2, 1> rx_enable;
        BitField<u16, u16, 4, 1> acknowledge;
        BitField<u16, bool, 6, 1> reset;
        BitField<u16, u16, 8, 2> rq_int_mode;
        BitField<u16, bool, 10, 1> tx_int_enable;
        BitField<u16, bool, 11, 1> rx_int_enable;
        BitField<u16, bool, 12, 1> ack_int_enable;
        BitField<u16, u16, 13, 1> slot_number;
        u16 bits = 0;
    } control;

    void Reset();

    u16 baudrate_reload = 0;

    Controller controller;

    System* sys = nullptr;
};
