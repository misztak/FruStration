#pragma once

#include <array>
#include <unordered_map>

#include "util/types.h"

class System;

class Debugger {
public:
    ALWAYS_INLINE bool IsBreakpoint(u32 address) {
        return unlikely(breakpoints.size() != 0 && breakpoints.count(address));
    }

    ALWAYS_INLINE bool IsBreakpointEnabled(u32 address) { return breakpoints.find(address)->second.enabled; }

    ALWAYS_INLINE bool IsWatchpoint(u32 address) { return unlikely(watchpoints.count(address)); }

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

    Debugger(System* system);
    void DrawDebugger(bool* open);
    void Reset();

    void AddBreakpoint(u32 address);
    void RemoveBreakpoint(u32 address);
    void ToggleBreakpoint(u32 address);

    void SetPausedState(bool paused, bool single_step);

    System* GetContext();

    bool single_step = false;
    bool single_frame = false;

    bool show_disasm_view = false;

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

    System* sys = nullptr;
};