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
        case 6:
            rgbc.r = value >> 0;
            rgbc.g = value >> 8;
            rgbc.b = value >> 16;
            rgbc.t = value >> 24;
            break;
        case 8:
            ir0 = static_cast<s16>(value);
            break;
        case 9:
            ir_vec.ir1 = static_cast<s16>(value);
            break;
        case 10:
            ir_vec.ir2 = static_cast<s16>(value);
            break;
        case 11:
            ir_vec.ir3 = static_cast<s16>(value);
            break;
        case 24:
            mac0 = static_cast<s32>(value);
            break;
        case 25:
            mac_vec.mac1 = static_cast<s32>(value);
            break;
        case 26:
            mac_vec.mac2 = static_cast<s32>(value);
            break;
        case 27:
            mac_vec.mac3 = static_cast<s32>(value);
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
        case 40:
            light_matrix.SetPairFromOffset(value, 0);
            break;
        case 41:
            light_matrix.SetPairFromOffset(value, 2);
            break;
        case 42:
            light_matrix.SetPairFromOffset(value, 4);
            break;
        case 43:
            light_matrix.SetPairFromOffset(value, 6);
            break;
        case 44:
            light_matrix[2][2] = static_cast<s16>(value);
            break;
        case 45:
            background_color.r = static_cast<s32>(value);
            break;
        case 46:
            background_color.g = static_cast<s32>(value);
            break;
        case 47:
            background_color.b = static_cast<s32>(value);
            break;
        case 48:
            color_matrix.SetPairFromOffset(value, 0);
            break;
        case 49:
            color_matrix.SetPairFromOffset(value, 2);
            break;
        case 50:
            color_matrix.SetPairFromOffset(value, 4);
            break;
        case 51:
            color_matrix.SetPairFromOffset(value, 6);
            break;
        case 52:
            color_matrix[2][2] = static_cast<s16>(value);
            break;
        case 53:
            far_color.r = static_cast<s32>(value);
            break;
        case 54:
            far_color.g = static_cast<s32>(value);
            break;
        case 55:
            far_color.b = static_cast<s32>(value);
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
        case 6:
            value = (u32(rgbc.r) << 0 | u32(rgbc.g) << 8 | u32(rgbc.b) << 16 | u32(rgbc.t) << 24);
            break;
        case 8:
            value = SignExtend32(ir0);
            break;
        case 9:
            value = SignExtend32(ir_vec.ir1);
            break;
        case 10:
            value = SignExtend32(ir_vec.ir2);
            break;
        case 11:
            value = SignExtend32(ir_vec.ir3);
            break;
        case 24:
            value = static_cast<u32>(mac0);
            break;
        case 25:
            value = static_cast<u32>(mac_vec.mac1);
            break;
        case 26:
            value = static_cast<u32>(mac_vec.mac2);
            break;
        case 27:
            value = static_cast<u32>(mac_vec.mac3);
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
        case 40:
            value = light_matrix.GetPairFromOffset(0);
            break;
        case 41:
            value = light_matrix.GetPairFromOffset(2);
            break;
        case 42:
            value = light_matrix.GetPairFromOffset(4);
            break;
        case 43:
            value = light_matrix.GetPairFromOffset(6);
            break;
        case 44:
            value = SignExtend32(light_matrix[2][2]);
            break;
        case 45:
            value = static_cast<u32>(background_color.r);
            break;
        case 46:
            value = static_cast<u32>(background_color.g);
            break;
        case 47:
            value = static_cast<u32>(background_color.b);
            break;
        case 48:
            value = color_matrix.GetPairFromOffset(0);
            break;
        case 49:
            value = color_matrix.GetPairFromOffset(2);
            break;
        case 50:
            value = color_matrix.GetPairFromOffset(4);
            break;
        case 51:
            value = color_matrix.GetPairFromOffset(6);
            break;
        case 52:
            value = SignExtend32(color_matrix[2][2]);
            break;
        case 53:
            value = static_cast<u32>(far_color.r);
            break;
        case 54:
            value = static_cast<u32>(far_color.g);
            break;
        case 55:
            value = static_cast<u32>(far_color.b);
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

void GTE::ExecuteCommand(u32 cmd_value) {
    LogDebug("COMMAND 0x{:02X}", cmd_value);

    Command cmd { .value = cmd_value };

    switch (cmd.real_opcode) {
        case 0x12: MatrixVectorMultiplication(cmd);
            break;
        default:
            Panic("Unimplemented Command: 0x{:02X}", cmd.real_opcode);
    }
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

template<u32 mac_id>
s64 GTE::CheckMacAndSignExtend(s64 value) {
    static_assert(mac_id > 0 && mac_id < 4);

    CheckMacOverflow<mac_id>(value);
    return SignExtendN<44>(value);
}

template<u32 ir_id>
void GTE::SetIR(s32 value, bool lm) {
    static_assert(ir_id > 0 && ir_id < 4);

    if constexpr (ir_id == 1) ir_vec.ir1 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
    if constexpr (ir_id == 2) ir_vec.ir2 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
    if constexpr (ir_id == 3) ir_vec.ir3 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
}

template<u32 mac_id>
s64 GTE::SetMac(s64 value, u8 shift) {
    static_assert(mac_id > 0 && mac_id < 4);

    CheckMacOverflow<mac_id>(value);

    value >>= shift;
    if constexpr (mac_id == 1) mac_vec.mac1 = static_cast<s32>(value);
    if constexpr (mac_id == 2) mac_vec.mac2 = static_cast<s32>(value);
    if constexpr (mac_id == 3) mac_vec.mac3 = static_cast<s32>(value);

    return value;
}

template<u32 id>
void GTE::SetMacAndIR(s64 value, u8 shift, bool lm) {
    static_assert(id > 0 && id < 4);

    s32 mac_shifted = static_cast<s32>(SetMac<id>(value, shift));
    SetIR<id>(mac_shifted, lm);
}

void GTE::MatrixVectorMultiplication(GTE::Command cmd) {
    LogDebug("MVMVA: {} * {} + {}", cmd.mvmva_m_mat, cmd.mvmva_m_vec, cmd.mvmva_t_vec);

    static constexpr Vector3<s32> t_zero {};

    ResetErrorFlag();

    // we deliberately keep this matrix uninitialized because it will *most likely* never be used
    // so don't waste time zero-initializing it during every invocation
    Matrix3x3 garbage_m;

    const Matrix3x3* m = nullptr;
    switch (cmd.mvmva_m_mat) {
        case 0: m = &rot_matrix; break;
        case 1: m = &light_matrix; break;
        case 2: m = &color_matrix; break;
        case 3:
        {
            garbage_m[0][0] = -static_cast<s16>(ZeroExtend16(rgbc.r << 4));
            garbage_m[0][1] = static_cast<s16>(ZeroExtend16(rgbc.r << 4));
            garbage_m[0][2] = ir0;
            garbage_m[1][0] = garbage_m[1][1] = garbage_m[1][2] = rot_matrix[0][2];
            garbage_m[2][0] = garbage_m[2][1] = garbage_m[2][2] = rot_matrix[1][1];
            m = &garbage_m;
            break;
        }
    }

    const Vector3<s16>* v = nullptr;
    switch (cmd.mvmva_m_vec) {
        case 0: v = &vec0; break;
        case 1: v = &vec1; break;
        case 2: v = &vec2; break;
        case 3: v = &ir_vec; break;
    }

    const Vector3<s32>* t = nullptr;
    switch (cmd.mvmva_t_vec) {
        case 0: t = &tl_vec; break;
        case 1: t = &background_color; break;
        case 2: t = &far_color; break;
        case 3: t = &t_zero; break;
    }

    u8 shift = cmd.sf ? 12 : 0;

    if (cmd.mvmva_t_vec != 2) {
        MVMVAKernel(m, v, t, shift, cmd.lm);
    } else {
        // bugged version if the far color vector was selected
        MVMVAKernelBugged(m, v, t, shift, cmd.lm);
    }
}

#define CMASE1(val) CheckMacAndSignExtend<1>(val)
#define CMASE2(val) CheckMacAndSignExtend<2>(val)
#define CMASE3(val) CheckMacAndSignExtend<3>(val)

void GTE::MVMVAKernel(const Matrix3x3* m, const Vector3<s16>* v, const Vector3<s32>* t, u8 shift, bool lm) {
    s64 x = CMASE1(CMASE1(CMASE1((s64(t->x) << 12) + s64(m->elems[0][0]) * s64(v->x)) + s64(m->elems[0][1]) * s64(v->y)) + s64(m->elems[0][2]) * s64(v->z));
    s64 y = CMASE2(CMASE2(CMASE2((s64(t->y) << 12) + s64(m->elems[1][0]) * s64(v->x)) + s64(m->elems[1][1]) * s64(v->y)) + s64(m->elems[1][2]) * s64(v->z));
    s64 z = CMASE3(CMASE3(CMASE3((s64(t->z) << 12) + s64(m->elems[2][0]) * s64(v->x)) + s64(m->elems[2][1]) * s64(v->y)) + s64(m->elems[2][2]) * s64(v->z));

    SetMacAndIR<1>(x, shift, lm);
    SetMacAndIR<2>(y, shift, lm);
    SetMacAndIR<3>(z, shift, lm);
}

void GTE::MVMVAKernelBugged(const Matrix3x3* m, const Vector3<s16>* v, const Vector3<s32>* t, u8 shift, bool lm) {
    // the first component updates the flag
    SetIR<1>(static_cast<s16>(CMASE1(((s64(t->x) << 12) + s64(m->elems[0][0]) * s64(v->x)) >> shift)), lm);
    SetIR<2>(static_cast<s16>(CMASE2(((s64(t->y) << 12) + s64(m->elems[1][0]) * s64(v->x)) >> shift)), lm);
    SetIR<3>(static_cast<s16>(CMASE3(((s64(t->z) << 12) + s64(m->elems[2][0]) * s64(v->x)) >> shift)), lm);

    // result is calculated from components 2 and 3
    s64 x = CMASE1(CMASE1(s64(m->elems[0][1]) * s64(v->y)) + s64(m->elems[0][2]) * s64(v->z));
    s64 y = CMASE2(CMASE2(s64(m->elems[1][1]) * s64(v->y)) + s64(m->elems[1][2]) * s64(v->z));
    s64 z = CMASE3(CMASE3(s64(m->elems[2][1]) * s64(v->y)) + s64(m->elems[2][2]) * s64(v->z));

    SetMacAndIR<1>(x, shift, lm);
    SetMacAndIR<2>(y, shift, lm);
    SetMacAndIR<3>(z, shift, lm);
}

#undef CMASE1
#undef CMASE2
#undef CMASE3

void GTE::Reset() {
    error_flags.bits = 0;

    vec0 = {.x = 0, .y = 0, .z = 0};
    vec1 = {.x = 0, .y = 0, .z = 0};
    vec2 = {.x = 0, .y = 0, .z = 0};

    rgbc = {.r = 0, .g = 0, .b = 0, .t = 0};

    mac0 = 0;
    mac_vec = {.mac1 = 0, .mac2 = 0, .mac3 = 0};

    ir0 = 0;
    ir_vec = {.x = 0, .y = 0, .z = 0};

    rot_matrix = {};

    tl_vec = {.x = 0, .y = 0, .z = 0};

    light_matrix = {};

    background_color = {.r = 0, .g = 0, .b = 0};

    color_matrix = {};

    far_color = {.r = 0, .g = 0, .b = 0};

    sof_x = 0;
    sof_y = 0;

    proj_plane_dist = 0;

    z_scale_factor_3 = 0;
    z_scale_factor_4 = 0;

    depth_queue_param_A = 0;
    depth_queue_param_B = 0;

}
