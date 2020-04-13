#include "dma.h"

#include "bus.h"

DMA::DMA() {}

void DMA::Init(BUS* b) { bus = b; }

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

    Panic("Invalid DMA register");
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
        if (channel[channel_index].ready()) StartTransfer(channel_index);
        return;
    }

    Panic("Invalid DMA register");
}

// Commonly used DMA Control Register values for starting DMA transfers
//
// DMA0 MDEC.IN  01000201h (always)
// DMA1 MDEC.OUT 01000200h (always)
// DMA2 GPU      01000200h (VramRead), 01000201h (VramWrite), 01000401h (List)
// DMA3 CDROM    11000000h (normal), 11400100h (chopped, rarely used)
// DMA4 SPU      01000201h (write), 01000200h (read, rarely used)
// DMA5 PIO      N/A       (not used by any known games)
// DMA6 OTC      11000002h (always)

void DMA::StartTransfer(u32 channel_index) {
    auto dir = static_cast<Direction>(channel[channel_index].control.transfer_direction);
    // TODO: write a better log message
    printf("Starting DMA transfer to %s on channel %u in mode %u starting at address 0x%08X\n",
           (dir == Direction::ToRAM) ? "RAM" : "device", channel_index,
           (u32)channel[channel_index].control.sync_mode.GetValue(), channel[channel_index].base_address);
    // TODO: chopping and priority handling

    switch (dir) {
        case Direction::ToRAM:
            TransferToRAM(channel_index);
            break;
        case Direction::ToDevice:
            TransferToDevice(channel_index);
            break;
    }

    printf("Finished DMA transfer\n");
}

void DMA::TransferToRAM(u32 index) {
    auto& ch = channel[index];
    s32 step = (ch.control.mem_address_step == Step::Inc) ? 4 : -4;
    u32 address = ch.base_address;

    u32 transfer_count = 0;
    switch (ch.control.sync_mode) {
        case SyncMode::Manual:
            transfer_count = ch.bcr.word_count;
            break;
        case SyncMode::Request:
            transfer_count = ch.bcr.block_count * ch.bcr.block_size;
            break;
        case SyncMode::LinkedList:
            Panic("DMA SyncMode LinkedList not implemented");
            break;
    }

    auto channel_type = static_cast<DMA_Channel>(index);
    while (transfer_count > 0) {
        // align address and wrap it
        u32 current_address = address & 0x1FFFFC;
        u32 data = 0;

        switch (channel_type) {
            case DMA_Channel::MDECin:
            case DMA_Channel::MDECout:
            case DMA_Channel::GPU:
            case DMA_Channel::CDROM:
            case DMA_Channel::SPU:
            case DMA_Channel::PIO:
                Panic("DMA Channel %u not implemented", index);
                break;
            case DMA_Channel::OTC:
                // create an empty linked list, does not actually transfer data from any device
                if (transfer_count == 1)
                    // tail marker
                    data = 0xFFFFFF;
                else
                    // pointer to previous element
                    data = (address - 4) & 0x1FFFFF;
                break;
        }

        bus->Store<u32>(current_address, data);

        address += step;
        transfer_count--;
    }

    // TODO: reset start_trigger at DMA start
    ch.control.start_trigger = false;
    ch.control.start_busy = false;
    // TODO: reset other values (interrupts?)
}

void DMA::TransferToDevice(u32 index) {
    auto& ch = channel[index];
    s32 step = (ch.control.mem_address_step == Step::Inc) ? 4 : -4;
    // align address and wrap it
    u32 address = ch.base_address & 0x1FFFFC;

    u32 transfer_count = 0;
    switch (ch.control.sync_mode) {
        case SyncMode::Manual:
            transfer_count = ch.bcr.word_count;
            break;
        case SyncMode::Request:
            transfer_count = ch.bcr.block_count * ch.bcr.block_size;
            break;
        case SyncMode::LinkedList:
            // only allow GPU channel (for now?)
            Assert(index == 2);
            transfer_count = 1;
            break;
    }

    auto channel_type = static_cast<DMA_Channel>(index);
    while (transfer_count > 0) {
        switch (channel_type) {
            case DMA_Channel::MDECin:
            case DMA_Channel::MDECout:
                Panic("DMA Channel %u not implemented", index);
                break;
            case DMA_Channel::GPU:
                if (ch.control.sync_mode == SyncMode::LinkedList) {
                    u32 header = bus->Load<u32>(address);
                    u32 element_count = header >> 24;

                    while (element_count > 0) {
                        address = (address + 4) & 0x1FFFFC;
                        u32 gpu_command = bus->Load<u32>(address);
                        printf("DMA pushed command 0x%X to GPU\n", gpu_command);
                        element_count--;
                    }
                    // keep the loop going if the tail marker was not reached
                    if ((header & 0x800000) == 0) transfer_count++;
                    // jump to the next element of the linked list
                    address = header & 0x1FFFFC;
                } else {
                    u32 gpu_data = bus->Load<u32>(address);
                    printf("DMA pushed data 0x%X to GPU\n", gpu_data);
                }
                break;
            case DMA_Channel::CDROM:
            case DMA_Channel::SPU:
            case DMA_Channel::PIO:
                Panic("DMA Channel %u not implemented", index);
                break;
            case DMA_Channel::OTC:
                Panic("Tried to start DMA Transfer to device on channel 6 (OTC)");
                break;
        }

        if (ch.control.sync_mode != SyncMode::LinkedList) address = (address + step) & 0x1FFFFC;
        transfer_count--;
    }

    // TODO: reset start_trigger at DMA start
    ch.control.start_trigger = false;
    ch.control.start_busy = false;
    // TODO: reset other values (interrupts?)
}

void DMA::UpdateIRQStatus() {
    // TODO: check if this is right
    interrupt.irq_master_flag =
        interrupt.force_irq || (interrupt.irq_master_enable && ((interrupt.irq_enable & interrupt.irq_ack) != 0));
}
