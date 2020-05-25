#include "dma.h"

#include "bus.h"
#include "gpu.h"

DMA::DMA() {}

void DMA::Init(BUS* b, GPU* g) {
    bus = b;
    gpu = g;
}

u32 DMA::Load(u32 address) {
#if 0
    static u32 dma_load_ctr = 0; dma_load_ctr++;
    printf("[%u] DMA IO Load [relative address 0x%X]\n", dma_load_ctr, address);
#endif

    if (address == 0x70) return control.value;
    if (address == 0x74) return interrupt.value;

    const u32 channel_index = (address & 0x70) >> 4;
    const u32 channel_address = address & 0xF;

    if (channel_address == 0x0) {
        return channel[channel_index].base_address;
    }
    if (channel_address == 0x4) {
        return channel[channel_index].bcr.value;
    }
    if (channel_address == 0x8) {
        return channel[channel_index].control.value;
    }

    Panic("Invalid DMA register");
    return 0;
}

void DMA::Store(u32 address, u32 value) {
#if 0
    printf("DMA IO Store [0x%X @ relative address 0x%X]\n", value, address);
#endif
    
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

    const u32 channel_index = (address & 0x70) >> 4;
    const u32 channel_address = address & 0xF;

    if (channel_address == 0x0) {
        channel[channel_index].base_address = value & 0xFF'FFFF;
        return;
    }
    if (channel_address == 0x4) {
        channel[channel_index].bcr.value = value;
        return;
    }
    if (channel_address == 0x8) {
        channel[channel_index].control.value = value;
        if (channel[channel_index].ready()) StartTransfer(channel_index);
        return;
    }

    Panic("Invalid DMA register");
}

void DMA::StartTransfer(u32 index) {
    auto dir = static_cast<Direction>(channel[index].control.transfer_direction);
    // TODO: write a better log message
    printf("Starting DMA transfer to %s on channel %u in mode %u starting at address 0x%08X\n",
           (dir == Direction::ToRAM) ? "RAM" : "device", index,
           (u32)channel[index].control.sync_mode.GetValue(), channel[index].base_address);
    // TODO: chopping and priority handling
    
    switch (channel[index].control.sync_mode) {
        case SyncMode::LinkedList:
            TransferLinkedList(index);
            break;
        case SyncMode::Manual:
        case SyncMode::Request:
            TransferBlock(index);
            break;
    }
}

void DMA::TransferBlock(u32 index) {
    auto& ch = channel[index];
    auto channel_type = static_cast<DMA_Channel>(index);
    s32 step = (ch.control.mem_address_step == Step::Inc) ? 4 : -4;

    u32 transfer_count = 0;
    switch (ch.control.sync_mode) {
        case SyncMode::Manual:
            transfer_count = (ch.bcr.word_count == 0) ? 0x10000 : ch.bcr.word_count;
            break;
        case SyncMode::Request:
            transfer_count = ch.bcr.block_count * ch.bcr.block_size;
            if (index == 2 && ch.control.transfer_direction == Direction::ToDevice)
                printf("Possible start of CopyImageToVRAM with size %u\n", transfer_count);
            break;
        case SyncMode::LinkedList:
            Panic("This should never be reached");
    }

    // TODO: decrement block_count
    // TODO: decrement MADR in Request and LinkedList mode
    u32 addr = ch.base_address;
    u32 data = 0;

    while (transfer_count > 0) {
        // align and wrap the address
        u32 curr_addr = addr & ADDR_MASK;

        switch (ch.control.transfer_direction) {
            case Direction::ToRAM:
                switch (channel_type) {
                    case DMA_Channel::MDECin:
                    case DMA_Channel::MDECout:
                    case DMA_Channel::GPU:
                    case DMA_Channel::CDROM:
                    case DMA_Channel::SPU:
                    case DMA_Channel::PIO:
                        Panic("DMA block transfer for channel %u not implemented", index);
                        break;
                    case DMA_Channel::OTC:
                        data = (transfer_count == 1) ? 0xFFFFFF : ((addr - 4) & 0x1FFFFF);
                        break;
                }
                bus->Store<u32>(curr_addr, data);
                break;
            case Direction::ToDevice:
                data = bus->Load<u32>(curr_addr);
                switch (channel_type) {
                    case DMA_Channel::GPU:
                        gpu->SendGP0Cmd(data);
                        break;
                    case DMA_Channel::CDROM:
                    case DMA_Channel::SPU:
                    case DMA_Channel::PIO:
                    case DMA_Channel::OTC:
                    case DMA_Channel::MDECout:
                    case DMA_Channel::MDECin:
                        Panic("DMA block transfer for channel %u not implemented", index);
                }
        }

        addr += step;
        transfer_count--;
    }
    // TODO: reset start_trigger at DMA start
    // TODO: reset other values (interrupts?)
    ch.control.start_busy = false;
    ch.control.start_trigger = false;
}

void DMA::TransferLinkedList(u32 index) {
    auto& ch = channel[index];
    auto channel_type = static_cast<DMA_Channel>(index);

    if (channel_type != DMA_Channel::GPU || ch.control.transfer_direction == Direction::ToRAM) 
        Panic("DMA linked list mode only available for GPU channel in ToDevice mode");

    // align and wrap the address
    u32 addr = ch.base_address & ADDR_MASK;

    for (;;) {
        u32 header = bus->Load<u32>(addr);
        u32 transfer_size = header >> 24;

        while (transfer_size > 0) {
            addr = (addr + 4) & ADDR_MASK;
            u32 cmd = bus->Load<u32>(addr);
            gpu->SendGP0Cmd(cmd);
            transfer_size--;
        }

        if ((header & 0x800000) != 0) break;

        addr = header & ADDR_MASK;
    }
    // TODO: reset start_trigger at DMA start
    // TODO: reset other values (interrupts?)
    ch.control.start_busy = false;
    ch.control.start_trigger = false;
}

void DMA::UpdateIRQStatus() {
    // TODO: check if this is right
    interrupt.irq_master_flag =
        interrupt.force_irq || (interrupt.irq_master_enable && ((interrupt.irq_enable & interrupt.irq_flag) != 0));
}