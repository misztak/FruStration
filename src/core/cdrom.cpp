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
                state = State::ExecutingSecondResponse;
            }
            break;
        case State::ExecutingSecondResponse:
            //DebugAssert(pending_command == Command::None);
            DebugAssert(cycles_until_second_response >= cycles);

            cycles_until_second_response -= cycles;

            if (cycles_until_second_response == 0) {
                DebugAssert(second_response_command);
                second_response_command();

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

    cycles_until_second_response = cycles;

    second_response_command = command;
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
    LOG_DEBUG << fmt::format("Received command 0x{:02X}", command);

    DebugAssert(interrupt_fifo.empty());
    DebugAssert(response_fifo.empty());

    // TODO: set this during ScheduleFirstResponse?
    status.busy = true;

    switch (command) {
        case Command::GetStat:
            PushResponse(INT3, stat.value);
            stat.shell_opened = false;
            break;
        case Command::Setloc:
            PushResponse(INT3, stat.value);
            // TODO: set seek target
            break;
        case Command::SeekL:
            PushResponse(INT3, stat.value);
            Panic("SeekL: Unimplemented");
            break;
        case Command::Test:
            ExecSubCommand();
            break;
        case Command::GetID:
            PushResponse(INT3, stat.value);
            // no disc
            //ScheduleSecondResponse(
            //    [this] { PushResponse(INT5, {0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});});

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
        if (index == 0) {
            // received a new command
            DebugAssert(pending_command == Command::None);
            DebugAssert(pending_second_response_command == Command::None);
            // command 255 is used internally for Command::None
            DebugAssert(value != 255);

            sys->ForceUpdateComponents();

            pending_command = static_cast<Command>(value);
            ScheduleFirstResponse();

            sys->RecalculateCyclesUntilNextEvent();
        }
        if (index == 1 || index == 2 || index == 3) Panic("Unimplemented");
        return;
    }

    if (address == 0x2) {
        if (index == 0) parameter_fifo.push_back(value);
        if (index == 1) {
            sys->ForceUpdateComponents();

            // set interrupt enable
            interrupt_enable = value;
            // if an interrupt was waiting it can be sent now
            SendInterrupt();

            sys->RecalculateCyclesUntilNextEvent();
        }
        if (index == 2 || index == 3) Panic("Unimplemented");
        return;
    }

    if (address == 0x3) {
        if (index == 0) request = value;
        if (index == 1) {
            sys->ForceUpdateComponents();

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

    stat.value = 0;
    stat.motor_on = true;

    status.value = 0;
    request = 0;

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
