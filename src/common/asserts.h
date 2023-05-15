#pragma once

#include <cstdlib>

#include "log.h"

#define STR_(x) #x
#define STRINGIFY_(x) STR_(x)

#define Panic(Message, ...)                                                                       \
    {                                                                                             \
        LogCrit("PANIC: " #Message " at " FILE_BASENAME ":" STRINGIFY_(__LINE__), ##__VA_ARGS__); \
        Log::Shutdown();                                                                          \
        std::exit(1);                                                                             \
    }                                                                                             \
    while (0)

#define Assert(Expr)                                                                    \
    if (!(Expr)) {                                                                      \
        LogCrit("Assert failed: " #Expr " at " FILE_BASENAME ":" STRINGIFY_(__LINE__)); \
        Log::Shutdown();                                                                \
        std::exit(1);                                                                   \
    }                                                                                   \
    while (0)

#ifndef NDEBUG
#define DebugAssert(Expr)                                                               \
    if (!(Expr)) {                                                                      \
        LogCrit("Assert failed: " #Expr " at " FILE_BASENAME ":" STRINGIFY_(__LINE__)); \
        Log::Shutdown();                                                                \
        std::exit(1);                                                                   \
    }                                                                                   \
    while (0)
#else
#define DebugAssert(Expr) while (0)
#endif
