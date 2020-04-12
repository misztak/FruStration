#include "dma.h"

DMA::DMA() {}


u32 DMA::Load(u32 address) {
    printf("DMA IO Load [relative address 0x%X]\n", address);

    if (address == 0x70) return control.value;
    if (address == 0x74) return interrupt.value;

    const u32 channel_index = address >> 4;
    const u32 channel_address = address & 0xF;
    DebugAssert(channel_index <= 7);

    if (channel_address == 0x0) {
        return channel[channel_index].base_address;
    }
    if (channel_address == 0x4) {
        return channel[channel_index].bcr.value;
    }
    if (channel_address == 0x8) {
        return channel[channel_index].control.value;
    }

    Panic("Unimplemented DMA register");
    return 0;
}
void DMA::Store(u32 address, u32 value) {
    printf("DMA IO Store [0x%X @ relative address 0x%X]\n", value, address);

    if (address == 0x70) {
        control.value = value;
        return;
    }
    if (address == 0x74) {
        interrupt.value = (interrupt.value & ~WRITE_MASK) | (value & WRITE_MASK);
        // writing to channel ack resets it
        interrupt.value = interrupt.value & ~(value & RESET_ACK_MASK);
        UpdateIRQStatus();
        return;
    }

    const u32 channel_index = address >> 4;
    const u32 channel_address = address & 0xF;
    DebugAssert(channel_index <= 7);

    if (channel_address == 0x0) {
        channel[channel_index].base_address = value & 0x00FFFFFF;
        return;
    }
    if (channel_address == 0x4) {
        channel[channel_index].bcr.value = value;
        return;
    }
    if (channel_address == 0x8) {
        channel[channel_index].control.value = value;
        return;
    }

    Panic("Unimplemented DMA register");
}

void DMA::UpdateIRQStatus() {
    // TODO: check if this is right
    interrupt.irq_master_flag = interrupt.force_irq || (interrupt.irq_master_enable && ((interrupt.irq_enable & interrupt.irq_ack) != 0));
}
