#pragma once

#include <unordered_map>
#include <array>

#include "types.h"

namespace CPU {
class CPU;
}

class Debugger {
public:
    ALWAYS_INLINE bool IsBreakpoint(u32 address) {
        return unlikely(attached && breakpoints.count(address));
    }
    ALWAYS_INLINE bool BreakpointEnabled(u32 address) {
        return breakpoints.find(address)->second.enabled;
    }

    ALWAYS_INLINE bool IsWatchpoint(u32 address) {
        return unlikely(attached && watchpoints.count(address));
    }
    ALWAYS_INLINE bool WatchpointEnabledOnStore(u32 address) {
        auto& wp = watchpoints.find(address)->second;
        return wp.type == Watchpoint::ENABLED || wp.type == Watchpoint::ONLY_STORE;
    }
    ALWAYS_INLINE bool WatchpointEnabledOnLoad(u32 address) {
        auto& wp = watchpoints.find(address)->second;
        return wp.type == Watchpoint::ENABLED || wp.type == Watchpoint::ONLY_LOAD;
    }

    ALWAYS_INLINE void StoreLastInstruction(u32 address, u32 value) {
        last_instructions[ring_ptr] = std::make_pair(address, value);
        ring_ptr = (ring_ptr + 1) & 127;
    }

    void Init(CPU::CPU* cpu);
    void DrawDebugger(bool* open);
    void Reset();

    bool attached = false;
    bool single_step = false;
private:
    struct Breakpoint {
        bool enabled = true;
    };
    std::unordered_map<u32, Breakpoint> breakpoints;

    struct Watchpoint {
        enum Type { ENABLED, ONLY_LOAD, ONLY_STORE, DISABLED };
        Type type = ENABLED;
        const char* TypeToString() {
            switch (type) {
                case ENABLED: return "ENABLED";
                case ONLY_LOAD: return "ON_LOAD";
                case ONLY_STORE: return "ON_STORE";
                case DISABLED: return "DISABLED";
            }
            return "XXX";
        }

        Watchpoint(Type type) : type(type) {}
    };
    std::unordered_map<u32, Watchpoint> watchpoints;

    static constexpr u32 BUFFER_SIZE = 128;
    static constexpr u32 BUFFER_MASK = BUFFER_SIZE - 1;
    u32 ring_ptr = 0;
    std::array<std::pair<u32, u32>, BUFFER_SIZE> last_instructions;
    CPU::CPU* cpu = nullptr;
};