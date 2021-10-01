#include "cdrom.h"

#include "interrupt.h"
#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(CDROM);

CDROM::CDROM() {
    stat.motor_on = true;

    status.par_fifo_empty = true;
    status.par_fifo_not_full = true;
}

void CDROM::Init(InterruptController* icontroller) {
    interrupt_controller = icontroller;
}

void CDROM::Step() {
    status.busy = false;

    if (!interrupt_fifo.empty()) {
        // don't pop the interrupt until it gets acknowledged
        u8 type = interrupt_fifo.front();

        if (type & interrupt_enable) {
            interrupt_controller->Request(IRQ::CDROM);
        }
    }
}

void CDROM::ExecCommand(Command command) {
    LOG_DEBUG << fmt::format("Received command 0x{:02X}", command);

    Assert(interrupt_fifo.empty());
    status.busy = true;

    switch (command) {
        case Command::GetStat:
            PushResponse(INT3, {stat.value});
            stat.shell_opened = false;
            break;
        case Command::Test:
            ExecSubCommand();
            break;
        case Command::GetID:
            PushResponse(INT3, {stat.value});
            // no disc
            //PushResponse(INT5, {0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
            // licensed disc (NTSC)
            PushResponse(INT2, {0x02, 0x00, 0x20, 0x00, 'S', 'C', 'E', 'A'});
            break;
        default:
            Panic("Unimplemented CDROM command 0x%02X", static_cast<u8>(command));
    }

    // clear the parameter fifo and reset flags
    parameter_fifo.clear();
    status.par_fifo_empty = true;
    status.par_fifo_not_full = true;
}

void CDROM::ExecSubCommand() {
    u8 sub_command = parameter_fifo.front();
    LOG_DEBUG << fmt::format("Received test command 0x{:02X}", sub_command);

    switch (sub_command) {
        case 0x20:
            // CDROM controller version (PU-7, version vC0)
            PushResponse(INT3, {0x94, 0x09, 0x19, 0xC0});
            break;
        default:
            Panic("Unimplemented CDROM test command 0x%02X", sub_command);
    }
}

void CDROM::PushResponse(u8 type, std::initializer_list<u8> value) {
    DebugAssert(response_fifo.size() + value.size() < FIFO_MAX_SIZE);

    // add a new interrupt to the queue
    interrupt_fifo.push_back(type);

    // add response to queue
    response_fifo.insert(response_fifo.end(), value);
    status.res_fifo_not_empty = true;
}

u8 CDROM::Load(u32 address) {
    LOG_DEBUG << fmt::format("Load call to CDROM [@ 0x{:01X}]", address);

    if (address == 0x0) {
        return status.value;
    }

    if (address == 0x1) {
        if (response_fifo.empty()) {
            LOG_DEBUG << "Read from empty response fifo";
            // TODO: return old value if no new response arrived?
            return 0;
        } else {
            LOG_DEBUG << "Read from response fifo";
            u8 response_byte = response_fifo.front();
            response_fifo.pop_front();
            return response_byte;
        }
    }

    if (address == 0x3) {
        u8 result = 0b11100000;

        if (!interrupt_fifo.empty()) result |= interrupt_fifo.front();
        LOG_DEBUG << "Read from interrupt fifo";

        return result;
    }

    Panic("Unimplemented");

    return 0;
}

void CDROM::Store(u32 address, u8 value) {
    LOG_DEBUG << fmt::format("Store call to CDROM [0x{:02X} @ 0x{:01}]", value, address);

    if (address == 0x0) {
        // only the index field is writable
        status.index = value & 0x3;
        return;
    }

    u32 index = status.index;

    if (address == 0x1) {
        if (index == 0) ExecCommand(static_cast<Command>(value));
        if (index == 1 || index == 2 || index == 3) Panic("Unimplemented");
        return;
    }

    if (address == 0x2) {
        if (index == 0) parameter_fifo.push_back(value);
        if (index == 1) interrupt_enable = value;
        if (index == 2 || index == 3) Panic("Unimplemented");
        return;
    }

    if (address == 0x3) {
        if (index == 0) request = value;
        if (index == 1) {
            if (value & 0x40) {
                // reset parameter fifo
                parameter_fifo.clear();
                status.par_fifo_empty = true;
                status.par_fifo_not_full = true;
            }

            // acknowledge an IRQ
            if (value & 0x07) {
                DebugAssert(value == 0x07 || value == 0x1F);
                LOG_DEBUG << "Acknowledged last interrupt";
                if (!interrupt_fifo.empty()) interrupt_fifo.pop_front();
            }
        }
        if (index == 2 || index == 3) Panic("Unimplemented");
        return;
    }
}

u8 CDROM::Peek(u32 address) {
    if (address == 0x0) {
        return status.value;
    }
    if (address == 0x1) {
        return response_fifo.empty() ? 0 : response_fifo.front();
    }
    if (address == 0x3) {
        u8 result = 0b11100000;
        if (!interrupt_fifo.empty()) result |= interrupt_fifo.front();
        return result;
    }
    return 0;
}

void CDROM::Reset() {
    stat.value = 0;
    stat.motor_on = true;

    status.value = 0;
    request = 0;

    parameter_fifo.clear();
    response_fifo.clear();
    interrupt_fifo.clear();

    interrupt_enable = 0;

    status.par_fifo_empty = true;
    status.par_fifo_not_full = true;
}
