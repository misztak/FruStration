#include "gte.h"

#include "common/asserts.h"
#include "common/log.h"
#include "util/type_util.h"

LOG_CHANNEL(GTE);

template<>
void Vector3<s16>::SetXYFromU32(u32 value) {
    x = static_cast<s16>(value);
    y = static_cast<s16>(value >> 16);
}

template<>
u32 Vector3<s16>::GetXYAsU32() {
    u32 lsbs = static_cast<u32>(static_cast<u16>(x));
    u32 msbs = static_cast<u32>(static_cast<u16>(y));
    return (msbs << 16) | (lsbs & 0xFFFF);
}

template<typename ValueType>
u64 SignExtend44(ValueType value) {
    static constexpr u64 MASK = (u64(1) << 44) - 1;

    u64 val_64 = SignExtend64(value);
    return val_64 & MASK;
}

void GTE::ExecuteCommand(u32 cmd_value) {
    LogDebug("COMMAND 0x{:02X}", cmd_value);

    Command cmd { .value = cmd_value };

    //u8 sf = cmd.sf ? 12 : 0;

    switch (cmd.real_opcode) {
        case 0x12: MatrixVectorMultiplication(cmd);
            break;
        default:
            Panic("Unimplemented Command: 0x{:02X}", cmd.real_opcode);
    }
}

void GTE::SetReg(u32 index, u32 value) {
    DebugAssert(index < 64);

    switch (index) {
        case 0:
            vec0.SetXYFromU32(value);
            break;
        case 1:
            vec0.z = static_cast<s16>(value);
            break;
        case 2:
            vec1.SetXYFromU32(value);
            break;
        case 3:
            vec1.z = static_cast<s16>(value);
            break;
        case 4:
            vec2.SetXYFromU32(value);
            break;
        case 5:
            vec2.z = static_cast<s16>(value);
            break;
        case 32:
            rot_matrix.SetPairFromOffset(value, 0);
            break;
        case 33:
            rot_matrix.SetPairFromOffset(value, 2);
            break;
        case 34:
            rot_matrix.SetPairFromOffset(value, 4);
            break;
        case 35:
            rot_matrix.SetPairFromOffset(value, 6);
            break;
        case 36:
            rot_matrix[2][2] = static_cast<s16>(value);
            break;
        case 37:
            tl_vec.x = static_cast<s32>(value);
            break;
        case 38:
            tl_vec.y = static_cast<s32>(value);
            break;
        case 39:
            tl_vec.z = static_cast<s32>(value);
            break;
        case 56:
            sof_x = static_cast<s32>(value);
            break;
        case 57:
            sof_y = static_cast<s32>(value);
            break;
        case 58:
            proj_plane_dist = static_cast<u16>(value);
            break;
        case 59:
            depth_queue_param_A = static_cast<s16>(value);
            break;
        case 60:
            depth_queue_param_B = static_cast<s32>(value);
            break;
        case 61:
            z_scale_factor_3 = static_cast<s16>(value);
            break;
        case 62:
            z_scale_factor_4 = static_cast<s16>(value);
            break;
        default:
            Panic("Unimplemented SetReg register index: {}", index);
    }

    LogDebug("Set register {} to 0x{:08X}", index, value);
}

u32 GTE::GetReg(u32 index) {
    DebugAssert(index < 64);

    u32 value = 0;

    switch (index) {
        case 0:
            value = vec0.GetXYAsU32();
            break;
        case 1:
            value = SignExtend32(vec0.z);
            break;
        case 2:
            value = vec1.GetXYAsU32();
            break;
        case 3:
            value = SignExtend32(vec1.z);
            break;
        case 4:
            value = vec2.GetXYAsU32();
            break;
        case 5:
            value = SignExtend32(vec2.z);
            break;
        case 32:
            value = rot_matrix.GetPairFromOffset(0);
            break;
        case 33:
            value = rot_matrix.GetPairFromOffset(2);
            break;
        case 34:
            value = rot_matrix.GetPairFromOffset(4);
            break;
        case 35:
            value = rot_matrix.GetPairFromOffset(6);
            break;
        case 36:
            value = SignExtend32(rot_matrix[2][2]);
            break;
        case 37:
            value = static_cast<u32>(tl_vec.x);
            break;
        case 38:
            value = static_cast<u32>(tl_vec.y);
            break;
        case 39:
            value = static_cast<u32>(tl_vec.z);
            break;
        case 56:
            value = static_cast<u32>(sof_x);
            break;
        case 57:
            value = static_cast<u32>(sof_y);
            break;
        case 58:
            value = SignExtend32(proj_plane_dist);
            break;
        case 59:
            value = SignExtend32(depth_queue_param_A);
            break;
        case 60:
            value = SignExtend32(depth_queue_param_B);
            break;
        case 61:
            value = SignExtend32(z_scale_factor_3);
            break;
        case 62:
            value = SignExtend32(z_scale_factor_4);
            break;
        default:
            Panic("Unimplemented GetReg register index: {}", index);
    }

    LogDebug("Load from register {}: 0x{:08X}", index, value);

    return value;
}

template<u32 ir_id>
s32 GTE::SaturateIR(s32 value, bool lm) {
    static constexpr s32 MIN = ir_id == 0 ? 0x0000 : -0x8000;
    static constexpr s32 MAX = ir_id == 0 ? 0x1000 : 0x7FFF;

    s32 lower_bound = lm ? 0 : MIN;

    if (value < lower_bound) {
        if constexpr (ir_id == 0) error_flags.ir0_saturated = 1;
        if constexpr (ir_id == 1) error_flags.ir1_saturated = 1;
        if constexpr (ir_id == 2) error_flags.ir2_saturated = 1;
        if constexpr (ir_id == 3) error_flags.ir3_saturated = 1;
        return lower_bound;
    }

    if (value > MAX) {
        if constexpr (ir_id == 0) error_flags.ir0_saturated = 1;
        if constexpr (ir_id == 1) error_flags.ir1_saturated = 1;
        if constexpr (ir_id == 2) error_flags.ir2_saturated = 1;
        if constexpr (ir_id == 3) error_flags.ir3_saturated = 1;
        return MAX;
    }

    return value;
}

template<u32 color_id>
s32 GTE::SaturateColor(s32 value) {
    if (value < 0 || value > 255) {
        if constexpr (color_id == 0) error_flags.fifo_red_saturated = 1;
        if constexpr (color_id == 1) error_flags.fifo_green_saturated = 1;
        if constexpr (color_id == 2) error_flags.fifo_blue_saturated = 1;
    }

    return std::clamp(value, 0, 255);
}

template<u32 coord_id>
s32 GTE::SaturateScreenCoords(s32 value) {
    if (value < -1024 || value > 1023) {
        if constexpr (coord_id == 0) error_flags.sx2_saturated = 1;
        if constexpr (coord_id == 1) error_flags.sy2_saturated = 1;
    }

    return std::clamp(value, -1024, 1023);
}

template<u32 mac_id>
void GTE::CheckMacOverflow(s64 value) {
    static constexpr s64 MIN = -(s64(1) << ((mac_id == 0 ? 32 : 44) - 1));
    static constexpr s64 MAX = +(s64(1) << ((mac_id == 0 ? 32 : 44) - 1)) - 1;

    if (value < MIN) {
        if constexpr (mac_id == 0) error_flags.mac0_underflow = 1;
        if constexpr (mac_id == 1) error_flags.mac1_underflow = 1;
        if constexpr (mac_id == 2) error_flags.mac2_underflow = 1;
        if constexpr (mac_id == 3) error_flags.mac3_underflow = 1;
    }
    if (value > MAX) {
        if constexpr (mac_id == 0) error_flags.mac0_overflow = 1;
        if constexpr (mac_id == 1) error_flags.mac1_overflow = 1;
        if constexpr (mac_id == 2) error_flags.mac2_overflow = 1;
        if constexpr (mac_id == 3) error_flags.mac3_overflow = 1;
    }
}

void GTE::MatrixVectorMultiplication(GTE::Command cmd) {
    LogDebug("MVMVA: {} * {} + {}", cmd.mvmva_m_mat, cmd.mvmva_m_vec, cmd.mvmva_t_vec);

    auto GetMatrix = [this](Command cmd) -> Matrix3x3& {
        switch (cmd.mvmva_m_mat) {
            case 0: return rot_matrix;
            default: Panic("Unimplemented matrix type {}", cmd.mvmva_m_mat);
        }
    };

    Matrix3x3& mat = GetMatrix(cmd);

    auto GetVector = [this](Command cmd) -> Vector3<s16>& {
        switch (cmd.mvmva_m_vec) {
            case 0: return vec0;
            case 1: return vec1;
            case 2: return vec2;
            default: Panic("Unimplemented vector type 3");
        }
    };

    Vector3<s16> m_vec = GetVector(cmd);



    Panic("MVMVA");
}

void GTE::Reset() {
    error_flags.bits = 0;

    vec0.raw.fill(0);
    vec1.raw.fill(0);
    vec2.raw.fill(0);

    for (auto& row : rot_matrix.elems) row.fill(0);

    tl_vec.raw.fill(0);

    sof_x = 0;
    sof_y = 0;

    proj_plane_dist = 0;

    z_scale_factor_3 = 0;
    z_scale_factor_4 = 0;

    depth_queue_param_A = 0;
    depth_queue_param_B = 0;

}
