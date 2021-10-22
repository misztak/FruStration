#pragma once

#include <deque>
#include <functional>

#include "types.h"
#include "bitfield.h"

class InterruptController;

class CDROM {
public:
    CDROM();
    void Init(InterruptController* icontroller);
    void Reset();
    void Step();

    u8 Load(u32 address);
    u8 Peek(u32 address);
    void Store(u32 address, u8 value);

private:
    enum class Command : u8 {
        Sync,
        GetStat,
        Setloc,
        Play,
        Forward,
        Backward,
        ReadN,
        MotorOn,
        Stop,
        Pause,
        Init,
        Mute,
        Demute,
        Setfilter,
        Setmode,
        Getparam,
        GetlocL,
        GetlocP,
        SetSession,
        GetTN,
        GetTD,
        SeekL,
        SeekP,
        SetClock,
        GetClock,
        Test,
        GetID,
        ReadS,
        Reset,
        GetQ,
        ReadTOC,
        VideoCD,
        None = 255,
    };

    enum class State : u32 {
        Idle,
        ExecutingFirstResponse,
        ExecutingSecondResponse,
    };

    State state = State::Idle;

    static constexpr u32 FIFO_MAX_SIZE = 16;

    static constexpr u8 INT0 = 0;
    static constexpr u8 INT1 = 1;
    static constexpr u8 INT2 = 2;
    static constexpr u8 INT3 = 3;
    static constexpr u8 INT4 = 4;
    static constexpr u8 INT5 = 5;

    void ExecCommand(Command command);
    void ExecSubCommand();
    void PushResponse(u8 type, std::initializer_list<u8> response_values);
    void PushResponse(u8 type, u8 response_value);

    void SendInterrupt();

    void StepTmp(u32 cycles);
    u32 CyclesUntilNextEvent();

    void ScheduleFirstResponse();
    void ScheduleSecondResponse(std::function<void ()> command, s32 cycles);

    std::function<void ()> second_response_command = nullptr;

    u32 cycles_until_first_response = 0;
    u32 cycles_until_second_response = 0;

    Command pending_command = Command::None;
    Command pending_second_response_command = Command::None;

    union {
        BitField<u8, bool, 0, 1> error;
        BitField<u8, bool, 1, 1> motor_on;
        BitField<u8, bool, 2, 1> seek_error;
        BitField<u8, bool, 3, 1> id_error;
        BitField<u8, bool, 4, 1> shell_opened;
        BitField<u8, bool, 5, 1> read;
        BitField<u8, bool, 6, 1> seek;
        BitField<u8, bool, 7, 1> play;
        u8 value = 0;
    } stat;

    union {
        BitField<u8, u8, 0, 2> index;
        BitField<u8, bool, 2, 1> adp_busy;
        BitField<u8, bool, 3, 1> par_fifo_empty;
        BitField<u8, bool, 4, 1> par_fifo_not_full;
        BitField<u8, bool, 5, 1> res_fifo_not_empty;
        BitField<u8, bool, 6, 1> dat_fifo_not_empty;
        BitField<u8, bool, 7, 1> busy;
        u8 value = 0;
    } status;

    u8 request = 0;

    std::deque<u8> parameter_fifo;

    std::deque<u8> response_fifo;

    u8 interrupt_enable = 0;
    std::deque<u8> interrupt_fifo;

    InterruptController* interrupt_controller = nullptr;
};