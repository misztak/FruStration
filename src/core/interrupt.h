#pragma once

#include "util/bitfield.h"
#include "util/types.h"

enum class IRQ : u32 {
    VBLANK = (1 << 0),
    GPU = (1 << 1),
    CDROM = (1 << 2),
    DMA = (1 << 3),
    TIMER0 = (1 << 4),
    TIMER1 = (1 << 5),
    TIMER2 = (1 << 6),
    MEM_CARD = (1 << 7),
    SIO = (1 << 8),
    SPU = (1 << 9),
    CONTROLLER = (1 << 10),
    NONE = (1 << 11),
};

class System;

class InterruptController {
public:
    explicit InterruptController(System* system);
    void Reset();
    void Request(IRQ irq);

    void StoreStat(u32 value);
    u32 LoadStat();
    void StoreMask(u32 value);
    u32 LoadMask();

    static const char* GetInterruptName(IRQ irq) {
        switch (irq) {
            case IRQ::VBLANK: return "Vblank";
            case IRQ::GPU: return "GPU (GP0)";
            case IRQ::CDROM: return "CDROM";
            case IRQ::DMA: return "DMA";
            case IRQ::TIMER0: return "Timer 0";
            case IRQ::TIMER1: return "Timer 1";
            case IRQ::TIMER2: return "Timer 2";
            case IRQ::MEM_CARD: return "Mem Card";
            case IRQ::SIO: return "SIO";
            case IRQ::SPU: return "SPU";
            case IRQ::CONTROLLER: return "Controller";
            case IRQ::NONE: return "None";
            default: return "Invalid IRQ";
        }
    }

    IRQ last_irq = IRQ::NONE;
private:
    void UpdateCP0Interrupt();

    static constexpr u32 IRQ_MASK = 0x7FF;

    union Register {
        u32 value = 0;

        BitField<u32, bool, 0, 1> vblank;
        BitField<u32, bool, 1, 1> gpu;
        BitField<u32, bool, 2, 1> cdrom;
        BitField<u32, bool, 3, 1> dma;
        BitField<u32, bool, 4, 1> timer0;
        BitField<u32, bool, 5, 1> timer1;
        BitField<u32, bool, 6, 1> timer2;
        BitField<u32, bool, 7, 1> controller_and_mem_card;
        BitField<u32, bool, 8, 1> sio;
        BitField<u32, bool, 9, 1> spu;
        BitField<u32, bool, 10, 1> controller_lightpen;
    };

    Register stat;
    Register mask;

    System* sys = nullptr;
};