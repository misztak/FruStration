#pragma once

#include "util/bitfield.h"
#include "util/types.h"

class System;

class DMA {
public:
    DMA(System* system);
    void Reset();

    u32 Load(u32 address);
    u32 Peek(u32 address);
    void Store(u32 address, u32 value);

private:
    void UpdateMasterFlag();
    void StartTransfer(u32 channel);
    void TransferBlock(u32 channel);
    void TransferLinkedList(u32 channel);

    static constexpr u32 ADDR_MASK = 0x1F'FFFC;
    enum class DMA_Channel : u32 {
        MDECin = 0,
        MDECout = 1,
        GPU = 2,
        CDROM = 3,
        SPU = 4,
        PIO = 5,
        OTC = 6,
    };

    enum class SyncMode : u32 {
        Manual = 0,
        Request = 1,
        LinkedList = 2,
    };

    enum class Direction : u32 {
        ToRAM = 0,
        ToDevice = 1,
    };

    enum class Step : u32 {
        Inc = 0,
        Dec = 1,
    };

    struct Channel {
        // only first 24 bits are used
        u32 base_address;

        union {
            u32 value;
            // Manual SyncMode
            BitField<u32, u32, 0, 16> word_count;
            // Request SyncMode
            BitField<u32, u32, 0, 16> block_size;
            BitField<u32, u32, 16, 16> block_count;
        } bcr;

        union {
            u32 value;

            BitField<u32, Direction, 0, 1> transfer_direction;
            BitField<u32, Step, 1, 1> mem_address_step;
            BitField<u32, bool, 8, 1> chopping_enable;
            BitField<u32, SyncMode, 9, 2> sync_mode;
            BitField<u32, u32, 16, 3> chopping_dma_win_size;
            BitField<u32, u32, 20, 3> chopping_cpu_win_size;
            BitField<u32, bool, 24, 1> start_busy;
            BitField<u32, bool, 28, 1> start_trigger;
            BitField<u32, bool, 29, 1> pause;
        } control;

        bool ready() {
            bool trigger = control.sync_mode != SyncMode::Manual || control.start_trigger;
            return control.start_busy && trigger;
        }
    };
    Channel channel[7] = {};

    union {
        u32 value = 0x07654321;

        BitField<u32, u32, 0, 3> mdec_in_prio;
        BitField<u32, u32, 3, 1> mdec_in_enable;
        BitField<u32, u32, 4, 3> mdec_out_prio;
        BitField<u32, u32, 7, 1> mdec_out_enable;
        BitField<u32, u32, 8, 3> gpu_prio;
        BitField<u32, u32, 11, 1> gpu_enable;
        BitField<u32, u32, 12, 3> cdrom_prio;
        BitField<u32, u32, 15, 1> cdrom_enable;
        BitField<u32, u32, 16, 3> spu_prio;
        BitField<u32, u32, 19, 1> spu_enable;
        BitField<u32, u32, 20, 3> pio_prio;
        BitField<u32, u32, 23, 1> pio_enable;
        BitField<u32, u32, 24, 3> otc_prio;
        BitField<u32, u32, 27, 1> otc_enable;
    } control;

    // first 6 bits unknown (R/W), 6..14 always zero
    // WRITE_MASK sets 15..23
    // RESET_ACK_MASK to reset 24..30
    // 31 is set through UpdateIRQStatus
    static constexpr u32 WRITE_MASK = 0x00FF803F;
    static constexpr u32 RESET_ACK_MASK = 0x7F000000;
    union {
        u32 value = 0;

        BitField<u32, bool, 15, 1> force_irq;
        BitField<u32, u32, 16, 7> irq_enable;
        BitField<u32, bool, 23, 1> irq_master_enable;
        BitField<u32, u32, 24, 7> irq_flag;
        BitField<u32, bool, 31, 1> irq_master_flag;
    } interrupt;

    System* sys = nullptr;
};