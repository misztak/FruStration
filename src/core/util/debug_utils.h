#pragma once

#include <cstdio>
#include <cstdlib>

#include "nano_log.h"

#define Panic(Message, ...)                                                                                    \
    {                                                                                                          \
        char msg[128];                                                                                         \
        std::snprintf(msg, sizeof(msg), Message, ##__VA_ARGS__);                                               \
        LOG_CRIT << "Panic: " << msg << " in function " << __FUNCTION__ << " (" << __FILE__ << ':' << __LINE__ \
                 << ")\n";                                                                                     \
        std::fflush(stderr);                                                                                   \
        std::exit(1);                                                                                          \
    }                                                                                                          \
    while (0)

#define Assert(Expr)                                                                                 \
    if (!(Expr)) {                                                                                   \
        std::fflush(stdout);                                                                         \
        LOG_CRIT << "Assert failed: '" #Expr "'"                                                     \
                 << " in function " << __FUNCTION__ << " (" << __FILE__ << ':' << __LINE__ << ")\n"; \
        std::fflush(stderr);                                                                         \
        std::exit(1);                                                                                \
    }                                                                                                \
    while (0)

#ifdef DEBUG
#define DebugAssert(Expr)                                                                            \
    if (!(Expr)) {                                                                                   \
        std::fflush(stdout);                                                                         \
        LOG_CRIT << "Assert failed: '" #Expr "'"                                                     \
                 << " in function " << __FUNCTION__ << " (" << __FILE__ << ':' << __LINE__ << ")\n"; \
        std::fflush(stderr);                                                                         \
        std::exit(1);                                                                                \
    }                                                                                                \
    while (0)
#else
#define DebugAssert(Expr) while (0)
#endif

#define AssertMsg(Expr, Msg)                                                                         \
    if (!(Expr)) {                                                                                   \
        std::fflush(stdout);                                                                         \
        LOG_CRIT << "Assert failed: '" #Msg "'"                                                      \
                 << " in function " << __FUNCTION__ << " (" << __FILE__ << ':' << __LINE__ << ")\n"; \
        std::fflush(stderr);                                                                         \
        std::exit(1);                                                                                \
    }                                                                                                \
    while (0)

// end
