#include "interrupt.h"

#include "common/asserts.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "system.h"

LOG_CHANNEL(IRQ);

InterruptController::InterruptController(System* system) : sys(system) {}

void InterruptController::Reset() {
    stat.value = 0;
    mask.value = 0;

    last_irq = IRQ::NONE;
}

void InterruptController::Request(IRQ irq) {
    last_irq = irq;

    stat.value |= (u32)irq;
    UpdateCP0Interrupt();
}

void InterruptController::StoreStat(u32 value) {
    u32 old_stat = stat.value;
    stat.value &= value;
    LogDebug("ISTAT ACK [0b{:011b}] --> [0b{:011b}]", old_stat & IRQ_MASK, stat.value & IRQ_MASK);
    UpdateCP0Interrupt();
}

u32 InterruptController::LoadStat() {
    //LOG_DEBUG << fmt::format("ISTAT READ [0b{:011b}]", stat.value & IRQ_MASK);
    return stat.value;
}

void InterruptController::StoreMask(u32 value) {
    LogDebug("IMASK SET [0b{:011b}] --> [0b{:011b}]", mask.value & IRQ_MASK, value & IRQ_MASK);
    // for now only allow interrupts that we can "handle"
    Assert((value & ~(0b1111111)) == 0);
    mask.value = value;
    UpdateCP0Interrupt();
}

u32 InterruptController::LoadMask() {
    //LOG_DEBUG << fmt::format("IMASK READ [0b{:011b}]", mask.value & IRQ_MASK);
    return mask.value;
}

void InterruptController::UpdateCP0Interrupt() {
    // the interrupt controller sets bit 10 of the cause register (3rd bit in IP field)
    constexpr u8 BIT_10 = static_cast<u8>(1u << 2);

    if (stat.value & mask.value)
        sys->cpu->cp.cause.IP |= BIT_10;
    else
        sys->cpu->cp.cause.IP &= ~BIT_10;
}
