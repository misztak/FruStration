#include "cdrom.h"

#include "fmt/format.h"

#include "debug_utils.h"
#include "interrupt.h"
#include "system.h"

LOG_CHANNEL(CDROM);

CDROM::CDROM(System* system): sys(system) {
    stat.motor_on = true;

    status.par_fifo_empty = true;
    status.par_fifo_not_full = true;

    cycles_until_first_response = MaxCycles;
    cycles_until_second_response = MaxCycles;
}

void CDROM::Step(u32 cycles) {
    switch (state) {
        case State::ExecutingFirstResponse:
            DebugAssert(pending_command != Command::None);
            DebugAssert(cycles_until_first_response >= cycles);

            cycles_until_first_response -= cycles;

            if (cycles_until_first_response == 0) {
                ExecCommand(pending_command);

                pending_command = Command::None;
                cycles_until_first_response = MaxCycles;
            }
            break;
        case State::ExecutingSecondResponse:
            DebugAssert(pending_command == Command::None);
            DebugAssert(cycles_until_second_response >= cycles);

            cycles_until_second_response -= cycles;

            if (cycles_until_second_response == 0) {
                DebugAssert(second_response_command);
                std::invoke(second_response_command);

                DebugAssert(!interrupt_fifo.empty());
                // if the first response interrupt is already acknowledged we can
                // trigger the second one immediately
                if (interrupt_fifo.size() == 1) SendInterrupt();

                pending_second_response_command = Command::None;
                second_response_command = nullptr;
                cycles_until_second_response = MaxCycles;
                state = State::Idle;
            }
            break;
        case State::Idle:
            break;
    }
}

u32 CDROM::CyclesUntilNextEvent() {
    return std::min(cycles_until_first_response, cycles_until_second_response);
}

void CDROM::ScheduleFirstResponse() {
    DebugAssert(pending_command != Command::None);

    // only start once last interrupt is acknowledged
    if (interrupt_fifo.empty()) {
        cycles_until_first_response = FIRST_RESPONSE_DELAY;
        state = State::ExecutingFirstResponse;
    }
}

void CDROM::ScheduleSecondResponse(std::function<void ()> command, s32 cycles = 0x0004a00) {
    //DebugAssert(cycles_until_second_response == MaxCycles);
    state = State::ExecutingSecondResponse;

    cycles_until_second_response = cycles;

    second_response_command = std::move(command);
}

void CDROM::SendInterrupt() {
    if (!interrupt_fifo.empty()) {
        // don't pop the interrupt until it gets acknowledged
        u8 type = interrupt_fifo.front();

        if (type & interrupt_enable) {
            sys->interrupt->Request(IRQ::CDROM);
        }
    }
}

void CDROM::ExecCommand(Command command) {
    DebugAssert(pending_second_response_command == Command::None);
    DebugAssert(interrupt_fifo.empty());

    // reset the state to Idle
    // if this command contains a second response the value
    // will be overwritten in ScheduleSecondResponse
    state = State::Idle;

    if (!response_fifo.empty())
        LOG_WARN << "Response fifo not empty, overwriting unread values!";

    response_fifo.clear();

    // TODO: update this stuff only during Load
    status.busy = true;

    switch (command) {
        case Command::GetStat:
            LOG_DEBUG << "GetStat";
            PushResponse(INT3, stat.value);
            stat.shell_opened = false;
            break;
        case Command::Setloc:
            LOG_DEBUG << "Setloc";
            DebugAssert(parameter_fifo.size() >= 3);
            minute = parameter_fifo[0];
            second = parameter_fifo[1];
            sector = parameter_fifo[2];
            PushResponse(INT3, stat.value);
            break;
        case Command::ReadN:
            LOG_DEBUG << "ReadN";
            PushResponse(INT3, stat.value);
            ScheduleSecondResponse([this] {
                ReadN();
            });
            break;
        case Command::Setmode:
            LOG_DEBUG << "Setmode";
            DebugAssert(parameter_fifo.size() >= 1 && parameter_fifo[0] == 0x80);
            mode.value = parameter_fifo[0];
            PushResponse(INT3, stat.value);
            break;
        case Command::SeekL:
            LOG_DEBUG << "SeekL";
            PushResponse(INT3, stat.value);
            ScheduleSecondResponse([this] {
                PushResponse(INT2, stat.value);
            });
            break;
        case Command::Test:
            LOG_DEBUG << "Test";
            ExecSubCommand();
            break;
        case Command::GetID:
            LOG_DEBUG << "GetID";
            PushResponse(INT3, stat.value);
            // no disc
            //ScheduleSecondResponse([this] {
            //    PushResponse(INT5, {0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
            //});

            // licensed disc (NTSC)
            ScheduleSecondResponse([this] {
                PushResponse(INT2, {0x02, 0x00, 0x20, 0x00, 'S', 'C', 'E', 'A'});
            });
            break;
        default:
            Panic("Unimplemented CDROM command 0x%02X", static_cast<u8>(command));
    }

    // clear the parameter fifo and reset flags
    parameter_fifo.clear();
    status.par_fifo_empty = true;
    status.par_fifo_not_full = true;

    // time to notify the cpu of the first response
    SendInterrupt();
}

void CDROM::ExecSubCommand() {
    u8 sub_command = parameter_fifo.front();

    switch (sub_command) {
        case 0x20:
            LOG_DEBUG << "CDROM controller version [PU-7, version vC0]";
            // CDROM controller version (PU-7, version vC0)
            PushResponse(INT3, {0x94, 0x09, 0x19, 0xC0});
            break;
        default:
            Panic("Unimplemented CDROM test command 0x%02X", sub_command);
    }
}

void CDROM::PushResponse(u8 type, std::initializer_list<u8> response_values) {
    DebugAssert(response_fifo.size() + response_values.size() < FIFO_MAX_SIZE);

    // add a new interrupt to the queue
    interrupt_fifo.push_back(type);

    // add response to queue
    response_fifo.insert(response_fifo.end(), response_values);
    status.res_fifo_not_empty = true;
}

void CDROM::PushResponse(u8 type, u8 response_value) {
    DebugAssert(response_fifo.size() + 1 < FIFO_MAX_SIZE);

    interrupt_fifo.push_back(type);

    response_fifo.push_back(response_value);
    status.res_fifo_not_empty = true;
}

u8 CDROM::Load(u32 address) {

    if (address == 0x0) {
        status.res_fifo_not_empty = !response_fifo.empty();
        LOG_DEBUG << fmt::format("Load: status [0b{:08b}]", status.value);
        return status.value;
    }

    if (address == 0x1) {
        if (response_fifo.empty()) {
            LOG_WARN << "Read from empty response fifo";
            // TODO: return old value or 0x00 depending on position
            return 0;
        } else {
            u8 response_byte = response_fifo.front();
            response_fifo.pop_front();
            LOG_DEBUG << fmt::format("Load: read from response fifo [0x{:02X}]", response_byte);
            return response_byte;
        }
    }

    if (address == 0x2) {
        Panic("Unimplemented");
    }

    if (address == 0x3) {
        switch (status.index) {
            case 0:
            case 2:
                Panic("Unimplemented");
                break;
            case 1:
            case 3:
            {
                u8 result = 0b11100000;

                if (!interrupt_fifo.empty()) result |= interrupt_fifo.front();
                LOG_DEBUG << fmt::format("Load: read from interrupt fifo [0b{:08b}]", result);

                return result;
            }
        }
    }

    Panic("Invalid CDROM register offset %u", address);
    return 0;
}

void CDROM::Store(u32 address, u8 value) {

    if (address == 0x0) {
        // only the index field is writable
        LOG_DEBUG << fmt::format("Store: changed register index from {} to {}", status.index, value & 0x3);
        status.index = value & 0x3;
        return;
    }

    u32 index = status.index;

    if (address == 0x1) {
        if (index == 0) {
            // received a new command
            DebugAssert(pending_command == Command::None);
            DebugAssert(pending_second_response_command == Command::None);
            // command 255 is used internally for Command::None
            DebugAssert(value != 255);

            sys->ForceUpdateComponents();

            LOG_DEBUG << fmt::format("Store: push new command 0x{:02X}", value);
            pending_command = static_cast<Command>(value);
            ScheduleFirstResponse();

            sys->RecalculateCyclesUntilNextEvent();
        }
        if (index == 1 || index == 2 || index == 3) Panic("Unimplemented");
        return;
    }

    if (address == 0x2) {
        if (index == 0) {
            LOG_DEBUG << fmt::format("Store: push parameter 0x{:02X}", value);
            parameter_fifo.push_back(value);
        }
        if (index == 1) {
            sys->ForceUpdateComponents();

            // set interrupt enable
            interrupt_enable = value;

            LOG_DEBUG << fmt::format("Store: interrupt enable -> 0b{:08b}", value);

            // if an interrupt was waiting it can be sent now
            SendInterrupt();

            sys->RecalculateCyclesUntilNextEvent();
        }
        if (index == 2 || index == 3) Panic("Unimplemented");
        return;
    }

    if (address == 0x3) {
        if (index == 0) {
            Panic("Unimplemented");
        }
        if (index == 1) {
            sys->ForceUpdateComponents();

            if (value & 0x40) {
                LOG_DEBUG << "Store: cleared parameter fifo";

                // reset parameter fifo
                parameter_fifo.clear();
                status.par_fifo_empty = true;
                status.par_fifo_not_full = true;
            }

            // acknowledge an IRQ
            if (value & 0x07) {
                DebugAssert(value == 0x07 || value == 0x1F);
                LOG_DEBUG << "Store: acknowledged last interrupt";

                if (!interrupt_fifo.empty()) {
                    interrupt_fifo.pop_front();
                    // second response available
                    if (!interrupt_fifo.empty()) {
                        SendInterrupt();
                    }
                }

                // a pending command waits for the fifo to be cleared
                // schedule it now if that is the case
                if (interrupt_fifo.empty() && pending_command != Command::None) {
                    ScheduleFirstResponse();
                }
            }

            sys->RecalculateCyclesUntilNextEvent();
        }
        if (index == 2 || index == 3) Panic("Unimplemented");
        return;
    }
}

void CDROM::ReadN() {

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
    state = State::Idle;

    mode.value = 0;

    stat.value = 0;
    stat.motor_on = true;

    status.value = 0;
    request = 0;

    minute = 0, second = 0, sector = 0;

    second_response_command = nullptr;

    cycles_until_first_response = MaxCycles;
    cycles_until_second_response = MaxCycles;

    pending_command = Command::None;
    pending_second_response_command = Command::None;

    parameter_fifo.clear();
    response_fifo.clear();
    interrupt_fifo.clear();

    interrupt_enable = 0;

    status.par_fifo_empty = true;
    status.par_fifo_not_full = true;
}
