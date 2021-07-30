#include "interrupt.h"

#include "cpu.h"
#include "fmt/format.h"
#include "macros.h"

LOG_CHANNEL(IRQ);

void InterruptController::Init(CPU::CPU* c) {
    cpu = c;
}

void InterruptController::Reset() {
    stat.value = 0;
    mask.value = 0;
}

void InterruptController::Request(IRQ irq) {
    // for now only allow interrupts that we can "handle"
    Assert(irq == IRQ::VBLANK || irq == IRQ::DMA);
    stat.value |= (u32)irq;
    UpdateCP0Interrupt();
}

void InterruptController::StoreStat(u32 value) {
    u32 old_stat = stat.value;
    stat.value &= value;
    LOG_DEBUG << fmt::format("ISTAT ACK [0b{:011b}] --> [0b{:011b}]", old_stat & IRQ_MASK, stat.value & IRQ_MASK);
    UpdateCP0Interrupt();
}

u32 InterruptController::LoadStat() {
    //LOG_DEBUG << fmt::format("ISTAT READ [0b{:011b}]", stat.value & IRQ_MASK);
    return stat.value;
}

void InterruptController::StoreMask(u32 value) {
    LOG_DEBUG << fmt::format("IMASK SET [0b{:011b}] --> [0b{:011b}]", mask.value & IRQ_MASK, value & IRQ_MASK);
    // for now only allow interrupts that we can "handle"
    Assert((value & ~(0b1101)) == 0);
    mask.value = value;
    UpdateCP0Interrupt();
}

u32 InterruptController::LoadMask() {
    //LOG_DEBUG << fmt::format("IMASK READ [0b{:011b}]", mask.value & IRQ_MASK);
    return mask.value;
}

void InterruptController::UpdateCP0Interrupt() {
    // the interrupt controller sets bit 10 of the cause register
    constexpr u8 BIT_INDEX = 2;

    if (stat.value & mask.value)
        cpu->cp.cause.IP |= static_cast<u8>((1U << BIT_INDEX));
    else
        cpu->cp.cause.IP &= static_cast<u8>(~(1U << BIT_INDEX));
}

