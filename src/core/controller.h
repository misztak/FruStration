#pragma once

#include <unordered_map>
#include <utility>

#include "util/types.h"

class Controller {
public:
    enum class Type : u16 {
        Mouse = 0x5A12,
        NegCon = 0x5A23,
        KonamiGun = 0x5A31,
        DigitalPad = 0x5A41,
        AnalogStick = 0x5A53,
        NamcoGun = 0x5A63,
        AnalogPad = 0x5A73,
        Multitap = 0x5A80,
        Jogcon = 0x5AE3,
        ConfigMode = 0x5AF3,
        NoController = 0xFFFF
    };

    enum class Button : u16 {
        Select      = 1 << 0,
        L3          = 1 << 1,
        R3          = 1 << 2,
        Start       = 1 << 3,
        Up          = 1 << 4,
        Right       = 1 << 5,
        Down        = 1 << 6,
        Left        = 1 << 7,
        L2          = 1 << 8,
        R2          = 1 << 9,
        L1          = 1 << 10,
        R1          = 1 << 11,
        Triangle    = 1 << 12,
        Circle      = 1 << 13,
        X           = 1 << 14,
        Square      = 1 << 15
    };

    using PSXKeyMap = std::unordered_map<u32, Button>;

    void Press(Button button);

    void SetKeyMap(PSXKeyMap map) {
        key_map = std::move(map);
    }

    PSXKeyMap& GetKeyMap() {
        return key_map;
    };

    Type ControllerType() {
        return type;
    }
private:
    static constexpr u8 HighZ = 0xFF;

    enum class State {
        Idle
    };

    // currently only digital pads are implemented
    Type type = Type::DigitalPad;

    State state = State::Idle;

    std::unordered_map<u32, Button> key_map;
};
