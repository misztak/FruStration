#pragma once

#include <type_traits>

#include "types.h"

template<typename ValueType, typename ReturnType>
ALWAYS_INLINE constexpr ReturnType SignExtend(ValueType value) {
    return static_cast<ReturnType>(
        static_cast<std::make_signed_t<ReturnType>>(static_cast<std::make_signed_t<ValueType>>(value)));
}

template<typename ValueType>
ALWAYS_INLINE constexpr u16 SignExtend16(ValueType value) {
    return SignExtend<ValueType, u16>(value);
}

template<typename ValueType>
ALWAYS_INLINE constexpr u32 SignExtend32(ValueType value) {
    return SignExtend<ValueType, u32>(value);
}

template<typename ValueType>
ALWAYS_INLINE constexpr u64 SignExtend64(ValueType value) {
    return SignExtend<ValueType, u64>(value);
}

template<typename ValueType, typename ReturnType>
ALWAYS_INLINE constexpr ReturnType ZeroExtend(ValueType value) {
    return static_cast<ReturnType>(
        static_cast<std::make_unsigned_t<ReturnType>>(static_cast<std::make_unsigned_t<ValueType>>(value)));
}

template<typename ValueType>
ALWAYS_INLINE constexpr u16 ZeroExtend16(ValueType value) {
    return ZeroExtend<ValueType, u16>(value);
}

template<typename ValueType>
ALWAYS_INLINE constexpr u32 ZeroExtend32(ValueType value) {
    return ZeroExtend<ValueType, u32>(value);
}

template<typename ValueType>
ALWAYS_INLINE constexpr u64 ZeroExtend64(ValueType value) {
    return ZeroExtend<ValueType, u64>(value);
}
