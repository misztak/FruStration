#pragma once

#include <cstdint>
#include <cstdio>

// force inline helper
#ifndef ALWAYS_INLINE
#if defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define ALWAYS_INLINE inline
#endif
#endif

#define ARRAYSIZE(ARR) ((int)(sizeof(ARR) / sizeof(*ARR)))

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

#define Panic(Message)                                                                                           \
    char buf[128];                                                                                               \
    std::snprintf(buf, sizeof(buf), "%s in function %s (%s:%u)", "Panic: '" Message "'", __FUNCTION__, __FILE__, \
                  __LINE__);                                                                                     \
    fputs(buf, stderr);                                                                                          \
    fflush(stderr);                                                                                              \
    abort();

#define Assert(Expr)                                                                                             \
    if (!(Expr)) {                                                                                               \
        char buf[128];                                                                                           \
        std::snprintf(buf, sizeof(buf), "%s in function %s (%s:%u)", "Assert failed: '" #Expr "'", __FUNCTION__, \
                      __FILE__, __LINE__);                                                                       \
        fputs(buf, stderr);                                                                                      \
        fflush(stderr);                                                                                          \
        abort();                                                                                                 \
    }

#define AssertMsg(Expr, Msg)                                                                                    \
    if (!(Expr)) {                                                                                              \
        char buf[128];                                                                                          \
        std::snprintf(buf, sizeof(buf), "%s in function %s (%s:%u)", "Assert failed: '" #Msg "'", __FUNCTION__, \
                      __FILE__, __LINE__);                                                                      \
        fputs(buf, stderr);                                                                                     \
        fflush(stderr);                                                                                         \
        abort();                                                                                                \
    }

// end