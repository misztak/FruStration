#pragma once

#include <cstdlib>

#include "log.h"

#define STR_(x) #x
#define STRINGIFY_(x) STR_(x)

#ifdef DEBUG
#define LOG_FILE_NAME __FILE__
#else
#define LOG_FILE_NAME __FILE_NAME__
#endif

#define Panic(Message, ...)                                                                       \
    {                                                                                             \
        LogCrit("PANIC: " #Message " at " LOG_FILE_NAME ":" STRINGIFY_(__LINE__), ##__VA_ARGS__); \
        Log::Shutdown();                                                                          \
        std::exit(1);                                                                             \
    }                                                                                             \
    while (0)

#define Assert(Expr)                                                                    \
    if (!(Expr)) {                                                                      \
        LogCrit("Assert failed: " #Expr " at " LOG_FILE_NAME ":" STRINGIFY_(__LINE__)); \
        Log::Shutdown();                                                                \
        std::exit(1);                                                                   \
    }                                                                                   \
    while (0)

#ifdef DEBUG
#define DebugAssert(Expr)                                                               \
    if (!(Expr)) {                                                                      \
        LogCrit("Assert failed: " #Expr " at " LOG_FILE_NAME ":" STRINGIFY_(__LINE__)); \
        Log::Shutdown();                                                                \
        std::exit(1);                                                                   \
    }                                                                                   \
    while (0)
#else
#define DebugAssert(Expr) while (0)
#endif
