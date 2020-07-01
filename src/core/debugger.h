#pragma once

#include <unordered_map>

#include "types.h"

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

    void DrawDebugger(bool* open);

    bool attached = false;
private:
    struct Breakpoint {
        bool enabled = true;
    };
    std::unordered_map<u32, Breakpoint> breakpoints;

    struct Watchpoint {
        enum Type { ENABLED, ONLY_LOAD, ONLY_STORE, DISABLED };
        Type type = ENABLED;
    };
    std::unordered_map<u32, Watchpoint> watchpoints;
};