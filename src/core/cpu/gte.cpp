#include "gte.h"

#include <bit>

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

    auto ColorFromU32 = [](u32 val, Color32& target) {
        target.r = val >> 0;
        target.g = val >> 8;
        target.b = val >> 16;
        target.t = val >> 24;
    };

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
            ColorFromU32(value, rgbc);
            break;
        case 7:
            otz = static_cast<u16>(value);
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
        case 12:
            screen[0].x = static_cast<s16>(value);
            screen[0].y = static_cast<s16>(value >> 16);
            break;
        case 13:
            screen[1].x = static_cast<s16>(value);
            screen[1].y = static_cast<s16>(value >> 16);
            break;
        case 14:
            screen[2].x = static_cast<s16>(value);
            screen[2].y = static_cast<s16>(value >> 16);
            break;
        case 15:
            screen[0].x = screen[1].x;
            screen[0].y = screen[1].y;
            screen[1].x = screen[2].x;
            screen[1].y = screen[2].y;
            screen[2].x = static_cast<s16>(value);
            screen[2].y = static_cast<s16>(value >> 16);
            break;
        case 16:
            screen_z[0] = static_cast<u16>(value);
            break;
        case 17:
            screen_z[1] = static_cast<u16>(value);
            break;
        case 18:
            screen_z[2] = static_cast<u16>(value);
            break;
        case 19:
            screen_z[3] = static_cast<u16>(value);
            break;
        case 20:
            ColorFromU32(value, rgb[0]);
            break;
        case 21:
            ColorFromU32(value, rgb[1]);
            break;
        case 22:
            ColorFromU32(value, rgb[2]);
            break;
        case 23:
            unused_reg = value;
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
        case 28:
            ir_vec.ir1 = static_cast<s16>((value >> 0 & 0x1F) * 0x80);
            ir_vec.ir2 = static_cast<s16>((value >> 5 & 0x1F) * 0x80);
            ir_vec.ir3 = static_cast<s16>((value >> 10 & 0x1F) * 0x80);
            break;
        case 29:
            break;
        case 30:
            leading_bit_source = static_cast<s32>(value);
            break;
        case 31:
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
        case 63:
            error_flags.bits = (error_flags.bits & 0x80000FFF) | (value & ~0x80000FFF);
            break;
        default:
            Panic("Unimplemented SetReg register index: {}", index);
    }

    //LogTrace("Set register {} to 0x{:08X}", index, value);
}

u32 GTE::GetReg(u32 index) {
    DebugAssert(index < 64);

    auto ColorToU32 = [](Color32& color) -> u32 {
        return (u32(color.r) << 0 | u32(color.g) << 8 | u32(color.b) << 16 | u32(color.t) << 24);
    };

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
            value = ColorToU32(rgbc);
            break;
        case 7:
            value = ZeroExtend32(otz);
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
        case 12:
            value = ZeroExtend32(screen[0].x) | (ZeroExtend32(screen[0].y) << 16);
            break;
        case 13:
            value = ZeroExtend32(screen[1].x) | (ZeroExtend32(screen[1].y) << 16);
            break;
        case 14:
            value = ZeroExtend32(screen[2].x) | (ZeroExtend32(screen[2].y) << 16);
            break;
        case 15:
            value = ZeroExtend32(screen[2].x) | (ZeroExtend32(screen[2].y) << 16);
            break;
        case 16:
            value = ZeroExtend32(screen_z[0]);
            break;
        case 17:
            value = ZeroExtend32(screen_z[1]);
            break;
        case 18:
            value = ZeroExtend32(screen_z[2]);
            break;
        case 19:
            value = ZeroExtend32(screen_z[3]);
            break;
        case 20:
            value = ColorToU32(rgb[0]);
            break;
        case 21:
            value = ColorToU32(rgb[1]);
            break;
        case 22:
            value = ColorToU32(rgb[2]);
            break;
        case 23:
            value = unused_reg;
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
        case 28:
        case 29:
        {
            u32 r = static_cast<u32>(std::clamp(ir_vec.ir1 / 0x80, 0x00, 0x1F));
            u32 g = static_cast<u32>(std::clamp(ir_vec.ir2 / 0x80, 0x00, 0x1F));
            u32 b = static_cast<u32>(std::clamp(ir_vec.ir3 / 0x80, 0x00, 0x1F));
            value = r | (g << 5) | (b << 10);
            break;
        }
        case 30:
            value = static_cast<u32>(leading_bit_source);
            break;
        case 31:
            value = static_cast<u32>(CountLeadingBits());
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
        case 63:
            UpdateErrMasterFlag();
            value = error_flags.bits;
            break;
        default:
            Panic("Unimplemented GetReg register index: {}", index);
    }

    //LogTrace("Load from register {}: 0x{:08X}", index, value);

    return value;
}

u32 GTE::CountLeadingBits() const {
    const u32 value = static_cast<u32>(leading_bit_source);

    if (value >> 31) {
        // count leading ones
        return static_cast<u32>(std::countl_one(value));
    } else {
        // count leading zeros
        return static_cast<u32>(std::countl_zero(value));
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
s32 GTE::SaturateScreenCoordsXY(s32 value) {
    static_assert(coord_id >= 0 && coord_id <= 1);

    if (value < -1024 || value > 1023) {
        if constexpr (coord_id == 0) error_flags.sx2_saturated = 1;
        if constexpr (coord_id == 1) error_flags.sy2_saturated = 1;
    }

    return std::clamp(value, -1024, 1023);
}

s32 GTE::SaturateScreenCoordsZ(s32 value) {
    if (value < 0x0000) value = 0x0000, error_flags.otz_saturated = 1;
    if (value > 0xFFFF) value = 0xFFFF, error_flags.otz_saturated = 1;
    return value;
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
    static_assert(ir_id >= 0 && ir_id < 4);

    if constexpr (ir_id == 0) ir0 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
    if constexpr (ir_id == 1) ir_vec.ir1 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
    if constexpr (ir_id == 2) ir_vec.ir2 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
    if constexpr (ir_id == 3) ir_vec.ir3 = static_cast<s16>(SaturateIR<ir_id>(value, lm));
}

s64 GTE::SetMac0(s64 value) {
    CheckMacOverflow<0>(value);

    mac0 = static_cast<s32>(value);
    return value;
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

void GTE::PushScreenX(s32 sx) {
    sx = SaturateScreenCoordsXY<0>(sx);

    screen[0].x = screen[1].x;
    screen[1].x = screen[2].x;
    screen[2].x = static_cast<s16>(sx);
}

void GTE::PushScreenY(s32 sy) {
    sy = SaturateScreenCoordsXY<1>(sy);

    screen[0].y = screen[1].y;
    screen[1].y = screen[2].y;
    screen[2].y = static_cast<s16>(sy);
}

void GTE::PushScreenZ(s32 sz) {
    sz = SaturateScreenCoordsZ(sz);

    screen_z[0] = screen_z[1];
    screen_z[1] = screen_z[2];
    screen_z[2] = screen_z[3];
    screen_z[3] = static_cast<u16>(sz);
}

void GTE::PushColor(s32 r, s32 g, s32 b) {
    r = SaturateColor<0>(r);
    g = SaturateColor<1>(g);
    b = SaturateColor<2>(b);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];
    rgb[2] = {.r = u8(r), .g = u8(g), .b = u8(b), .t = rgbc.t};
}

void GTE::PushColorFromMac() {
    PushColor(mac_vec.r / 0x10, mac_vec.g / 0x10, mac_vec.b / 0x10);
}

void GTE::SetOrderTableZ(s64 value) {
    static constexpr s64 MIN = 0x0000;
    static constexpr s64 MAX = 0xFFFF;

    value >>= 12; // div 0x1000

    if (value < MIN) {
        otz = static_cast<s16>(MIN);
        error_flags.otz_saturated = 1;
    } else if (value > MAX) {
        otz = static_cast<s16>(MAX);
        error_flags.otz_saturated = 1;
    } else {
        otz = static_cast<s16>(value);
    }
}

void GTE::InterpolateColor(s32 mac1, s32 mac2, s32 mac3, u8 shift, bool lm) {
    SetMacAndIR<1>((s64(far_color.r) << 12) - mac1, shift, false);
    SetMacAndIR<2>((s64(far_color.g) << 12) - mac2, shift, false);
    SetMacAndIR<3>((s64(far_color.b) << 12) - mac3, shift, false);

    SetMacAndIR<1>(s64(ir_vec.ir1) * s64(ir0) + mac1, shift, lm);
    SetMacAndIR<2>(s64(ir_vec.ir2) * s64(ir0) + mac2, shift, lm);
    SetMacAndIR<3>(s64(ir_vec.ir3) * s64(ir0) + mac3, shift, lm);
}

void GTE::ExecuteCommand(u32 cmd_value) {
    //LogTrace("COMMAND 0x{:02X}", cmd_value);

    ResetErrorFlag();

    Command cmd { .value = cmd_value };

    u8 shift = cmd.sf ? 12 : 0;
    bool lm = cmd.lm;

    switch (cmd.real_opcode) {
        case 0x01:
            RTPS(vec0, shift, lm, true);
            break;
        case 0x06:
            NCLIP();
            break;
        case 0x0C:
            OP(shift, lm);
            break;
        case 0x10:
            DPCS(shift, lm);
            break;
        case 0x11:
            INTPL(shift, lm);
            break;
        case 0x12:
            MVMVA(cmd);
            break;
        case 0x13:
            NCDS(shift, lm);
            break;
        case 0x14:
            CDP(shift, lm);
            break;
        case 0x16:
            NCDT(shift, lm);
            break;
        case 0x1B:
            NCCS(shift, lm);
            break;
        case 0x1C:
            CC(shift, lm);
            break;
        case 0x1E:
            NCS(shift, lm);
            break;
        case 0x20:
            NCT(shift, lm);
            break;
        case 0x28:
            SQR(shift, lm);
            break;
        case 0x29:
            DCPL(shift, lm);
            break;
        case 0x2A:
            DPCT(shift, lm);
            break;
        case 0x2D:
            AVSZ3();
            break;
        case 0x2E:
            AVSZ4();
            break;
        case 0x30:
            RTPT(shift, lm);
            break;
        case 0x3D:
            GPF(shift, lm);
            break;
        case 0x3E:
            GPL(shift, lm);
            break;
        case 0x3F:
            NCCT(shift, lm);
            break;
        default:
            Panic("Invalid GTE Command: 0x{:02X}", cmd.real_opcode);
    }

    UpdateErrMasterFlag();
}

void GTE::RTPS(const Vector3<s16>& v, u8 shift, bool lm, bool last_vertex) {
    s64 z = RTPKernel(v, shift, lm);

    PushScreenZ(static_cast<s32>(z >> 12));

    s64 div_result = static_cast<s64>(UNRDivide(proj_plane_dist, screen_z[3]));

    s32 screen_x = static_cast<s32>(SetMac0(div_result * ir_vec.ir1 + sof_x) >> 16); // SX = MAC0 / 0x10000
    s32 screen_y = static_cast<s32>(SetMac0(div_result * ir_vec.ir2 + sof_y) >> 16); // SY = MAC0 / 0x10000
    PushScreenX(screen_x);
    PushScreenY(screen_y);
    
    if (last_vertex) {
        s64 mac0_val = SetMac0(div_result * s64(depth_queue_param_A) + s64(depth_queue_param_B));
        SetIR<0>(static_cast<s32>(mac0_val >> 12), lm);
    }
}

void GTE::RTPT(u8 shift, bool lm) {
    RTPS(vec0, shift, lm, false);
    RTPS(vec1, shift, lm, false);
    RTPS(vec2, shift, lm, true);
}

void GTE::NCLIP() {
    s64 a = s64(screen[0].x) * s64(screen[1].y) + s64(screen[1].x) * s64(screen[2].y) + s64(screen[2].x) * s64(screen[0].y);
    s64 b = s64(screen[0].x) * s64(screen[2].y) + s64(screen[1].x) * s64(screen[0].y) + s64(screen[2].x) * s64(screen[1].y);
    SetMac0(a - b);
}

void GTE::SQR(u8 shift, bool lm) {
    SetMacAndIR<1>(s64(ir_vec.ir1) * s64(ir_vec.ir1), shift, lm);
    SetMacAndIR<2>(s64(ir_vec.ir2) * s64(ir_vec.ir2), shift, lm);
    SetMacAndIR<3>(s64(ir_vec.ir3) * s64(ir_vec.ir3), shift, lm);
}

void GTE::AVSZ3() {
    s64 avg = s64(z_scale_factor_3) * (screen_z[1] + screen_z[2] + screen_z[3]);
    SetMac0(avg);
    SetOrderTableZ(avg);
}

void GTE::AVSZ4() {
    s64 avg = s64(z_scale_factor_4) * (screen_z[0] + screen_z[1] + screen_z[2] + screen_z[3]);
    SetMac0(avg);
    SetOrderTableZ(avg);
}

void GTE::INTPL(u8 shift, bool lm) {
    // 'IRn << 12' cannot overflow the 44bit MAC, so no need to check
    InterpolateColor(s32(ir_vec.ir1) << 12, s32(ir_vec.ir2) << 12, s32(ir_vec.ir3) << 12, shift, lm);

    PushColorFromMac();
}

void GTE::OP(u8 shift, bool lm) {
    SetMac<1>(s64(rot_matrix[1][1]) * s64(ir_vec.ir3) - s64(rot_matrix[2][2] * ir_vec.ir2), shift);
    SetMac<2>(s64(rot_matrix[2][2]) * s64(ir_vec.ir1) - s64(rot_matrix[0][0] * ir_vec.ir3), shift);
    SetMac<3>(s64(rot_matrix[0][0]) * s64(ir_vec.ir2) - s64(rot_matrix[1][1] * ir_vec.ir1), shift);

    SetIR<1>(mac_vec.mac1, lm);
    SetIR<2>(mac_vec.mac2, lm);
    SetIR<3>(mac_vec.mac3, lm);
}

void GTE::MVMVA(GTE::Command cmd) {
    static constexpr Vector3<s32> t_zero {};

    // we deliberately keep this matrix uninitialized because it will *most likely* never be used
    // so don't waste time zero-initializing it during every invocation
    Matrix3x3 garbage_m;

    auto GetMatrix = [&]() -> const Matrix3x3& {
        switch (cmd.mvmva_m_mat) {
            case 0: return rot_matrix;
            case 1: return light_matrix;
            case 2: return color_matrix;
            case 3:
            {
                garbage_m[0][0] = -static_cast<s16>(ZeroExtend16(rgbc.r << 4));
                garbage_m[0][1] = static_cast<s16>(ZeroExtend16(rgbc.r << 4));
                garbage_m[0][2] = ir0;
                garbage_m[1][0] = garbage_m[1][1] = garbage_m[1][2] = rot_matrix[0][2];
                garbage_m[2][0] = garbage_m[2][1] = garbage_m[2][2] = rot_matrix[1][1];
                return garbage_m;
            }
        }
        __builtin_unreachable();
    };
    const Matrix3x3& m = GetMatrix();

    auto GetMulVec = [&]() -> const Vector3<s16>& {
        switch (cmd.mvmva_m_vec) {
            case 0: return vec0;
            case 1: return vec1;
            case 2: return vec2;
            case 3: return ir_vec;
        }
        __builtin_unreachable();
    };
    const Vector3<s16>& v = GetMulVec();

    auto GetTlVec = [&]() -> const Vector3<s32>& {
        switch (cmd.mvmva_t_vec) {
            case 0: return tl_vec;
            case 1: return background_color;
            case 2: return far_color;
            case 3: return t_zero;
        }
        __builtin_unreachable();
    };
    const Vector3<s32>& t = GetTlVec();

    u8 shift = cmd.sf ? 12 : 0;
    bool lm = cmd.lm;

    if (cmd.mvmva_t_vec != 2) {
        MatrixMultResult result = MatrixMultiply(m, v, t);
        SetMacAndIR<1>(result.x, shift, lm);
        SetMacAndIR<2>(result.y, shift, lm);
        SetMacAndIR<3>(result.z, shift, lm);
    } else {
        // bugged version if the far color vector was selected
        MVMVAKernelBugged(m, v, t, shift, cmd.lm);
    }
}

template<u32 type>
void GTE::NCKernel(const Vector3<s16>& v, u8 shift, bool lm) {
    const auto [x1, y1, z1] = MatrixMultiply(light_matrix, v);
    SetMacAndIR<1>(x1, shift, lm);
    SetMacAndIR<2>(y1, shift, lm);
    SetMacAndIR<3>(z1, shift, lm);

    const auto [x2, y2, z2] = MatrixMultiply(color_matrix, ir_vec, background_color);
    SetMacAndIR<1>(x2, shift, lm);
    SetMacAndIR<2>(y2, shift, lm);
    SetMacAndIR<3>(z2, shift, lm);

    if constexpr (type == 1) {
        SetMacAndIR<1>((s64(rgbc.r) * s64(ir_vec.ir1)) << 4, shift, lm);
        SetMacAndIR<2>((s64(rgbc.g) * s64(ir_vec.ir2)) << 4, shift, lm);
        SetMacAndIR<3>((s64(rgbc.b) * s64(ir_vec.ir3)) << 4, shift, lm);
    }

    if constexpr (type == 2) {
        s32 mac1 = (s32(rgbc.r) * s32(ir_vec.ir1)) << 4;
        s32 mac2 = (s32(rgbc.g) * s32(ir_vec.ir2)) << 4;
        s32 mac3 = (s32(rgbc.b) * s32(ir_vec.ir3)) << 4;

        InterpolateColor(mac1, mac2, mac3, shift, lm);
    }

    PushColorFromMac();
}

void GTE::NCS(u8 shift, bool lm) {
    NCKernel<0>(vec0, shift, lm);
}

void GTE::NCT(u8 shift, bool lm) {
    NCKernel<0>(vec0, shift, lm);
    NCKernel<0>(vec1, shift, lm);
    NCKernel<0>(vec2, shift, lm);
}

void GTE::NCCS(u8 shift, bool lm) {
    NCKernel<1>(vec0, shift, lm);
}

void GTE::NCCT(u8 shift, bool lm) {
    NCKernel<1>(vec0, shift, lm);
    NCKernel<1>(vec1, shift, lm);
    NCKernel<1>(vec2, shift, lm);
}

void GTE::NCDS(u8 shift, bool lm) {
    NCKernel<2>(vec0, shift, lm);
}

void GTE::NCDT(u8 shift, bool lm) {
    NCKernel<2>(vec0, shift, lm);
    NCKernel<2>(vec1, shift, lm);
    NCKernel<2>(vec2, shift, lm);
}

void GTE::CC(u8 shift, bool lm) {
    const auto [x, y, z] = MatrixMultiply(color_matrix, ir_vec, background_color);
    SetMacAndIR<1>(x, shift, lm);
    SetMacAndIR<2>(y, shift, lm);
    SetMacAndIR<3>(z, shift, lm);

    SetMacAndIR<1>((s64(rgbc.r) * s64(ir_vec.ir1)) << 4, shift, lm);
    SetMacAndIR<2>((s64(rgbc.g) * s64(ir_vec.ir2)) << 4, shift, lm);
    SetMacAndIR<3>((s64(rgbc.b) * s64(ir_vec.ir3)) << 4, shift, lm);

    PushColorFromMac();
}

void GTE::CDP(u8 shift, bool lm) {
    const auto [x, y, z] = MatrixMultiply(color_matrix, ir_vec, background_color);
    SetMacAndIR<1>(x, shift, lm);
    SetMacAndIR<2>(y, shift, lm);
    SetMacAndIR<3>(z, shift, lm);

    s32 mac1 = (s32(rgbc.r) * s32(ir_vec.ir1)) << 4;
    s32 mac2 = (s32(rgbc.g) * s32(ir_vec.ir2)) << 4;
    s32 mac3 = (s32(rgbc.b) * s32(ir_vec.ir3)) << 4;

    InterpolateColor(mac1, mac2, mac3, shift, lm);

    PushColorFromMac();
}

void GTE::DPCKernel(const GTE::Color32& color, u8 shift, bool lm) {
    SetMac<1>(s64(u64(color.r) << 16), 0);
    SetMac<2>(s64(u64(color.g) << 16), 0);
    SetMac<3>(s64(u64(color.b) << 16), 0);

    InterpolateColor(mac_vec.mac1, mac_vec.mac2, mac_vec.mac3, shift, lm);

    PushColorFromMac();
}

void GTE::DPCS(u8 shift, bool lm) {
    DPCKernel(rgbc, shift, lm);
}

void GTE::DPCT(u8 shift, bool lm) {
    DPCKernel(rgb[0], shift, lm);
    DPCKernel(rgb[0], shift, lm);
    DPCKernel(rgb[0], shift, lm);
}

void GTE::DCPL(u8 shift, bool lm) {
    s32 mac1 = (s32(rgbc.r) * s32(ir_vec.ir1)) << 4;
    s32 mac2 = (s32(rgbc.g) * s32(ir_vec.ir2)) << 4;
    s32 mac3 = (s32(rgbc.b) * s32(ir_vec.ir3)) << 4;

    InterpolateColor(mac1, mac2, mac3, shift, lm);

    PushColorFromMac();
}

void GTE::GPF(u8 shift, bool lm) {
    SetMacAndIR<1>(s64(ir_vec.ir1) * s64(ir0), shift, lm);
    SetMacAndIR<2>(s64(ir_vec.ir2) * s64(ir0), shift, lm);
    SetMacAndIR<3>(s64(ir_vec.ir3) * s64(ir0), shift, lm);

    PushColorFromMac();
}

void GTE::GPL(u8 shift, bool lm) {
    SetMacAndIR<1>(s64(ir_vec.ir1) * s64(ir0) + (mac_vec.mac1 << shift), shift, lm);
    SetMacAndIR<2>(s64(ir_vec.ir2) * s64(ir0) + (mac_vec.mac2 << shift), shift, lm);
    SetMacAndIR<3>(s64(ir_vec.ir3) * s64(ir0) + (mac_vec.mac3 << shift), shift, lm);

    PushColorFromMac();
}

#define CMASE1(val) CheckMacAndSignExtend<1>(val)
#define CMASE2(val) CheckMacAndSignExtend<2>(val)
#define CMASE3(val) CheckMacAndSignExtend<3>(val)

GTE::MatrixMultResult GTE::MatrixMultiply(const Matrix3x3& m, const Vector3<s16>& v) {
    s64 x = CMASE1(CMASE1(CMASE1(s64(m.elems[0][0]) * s64(v.x)) + s64(m.elems[0][1]) * s64(v.y)) + s64(m.elems[0][2]) * s64(v.z));
    s64 y = CMASE2(CMASE2(CMASE2(s64(m.elems[1][0]) * s64(v.x)) + s64(m.elems[1][1]) * s64(v.y)) + s64(m.elems[1][2]) * s64(v.z));
    s64 z = CMASE3(CMASE3(CMASE3(s64(m.elems[2][0]) * s64(v.x)) + s64(m.elems[2][1]) * s64(v.y)) + s64(m.elems[2][2]) * s64(v.z));

    return {.x = x, .y = y, .z = z};
}

GTE::MatrixMultResult GTE::MatrixMultiply(const Matrix3x3& m, const Vector3<s16>& v, const Vector3<s32>& t) {
    s64 x = CMASE1(CMASE1(CMASE1((s64(t.x) << 12) + s64(m.elems[0][0]) * s64(v.x)) + s64(m.elems[0][1]) * s64(v.y)) + s64(m.elems[0][2]) * s64(v.z));
    s64 y = CMASE2(CMASE2(CMASE2((s64(t.y) << 12) + s64(m.elems[1][0]) * s64(v.x)) + s64(m.elems[1][1]) * s64(v.y)) + s64(m.elems[1][2]) * s64(v.z));
    s64 z = CMASE3(CMASE3(CMASE3((s64(t.z) << 12) + s64(m.elems[2][0]) * s64(v.x)) + s64(m.elems[2][1]) * s64(v.y)) + s64(m.elems[2][2]) * s64(v.z));

    return {.x = x, .y = y, .z = z};
}

void GTE::MVMVAKernelBugged(const Matrix3x3& m, const Vector3<s16>& v, const Vector3<s32>& t, u8 shift, bool lm) {
    // the first component updates the flag
    SetIR<1>(static_cast<s16>(CMASE1(((s64(t.x) << 12) + s64(m.elems[0][0]) * s64(v.x)) >> shift)), lm);
    SetIR<2>(static_cast<s16>(CMASE2(((s64(t.y) << 12) + s64(m.elems[1][0]) * s64(v.x)) >> shift)), lm);
    SetIR<3>(static_cast<s16>(CMASE3(((s64(t.z) << 12) + s64(m.elems[2][0]) * s64(v.x)) >> shift)), lm);

    // result is calculated from components 2 and 3
    s64 x = CMASE1(CMASE1(s64(m.elems[0][1]) * s64(v.y)) + s64(m.elems[0][2]) * s64(v.z));
    s64 y = CMASE2(CMASE2(s64(m.elems[1][1]) * s64(v.y)) + s64(m.elems[1][2]) * s64(v.z));
    s64 z = CMASE3(CMASE3(s64(m.elems[2][1]) * s64(v.y)) + s64(m.elems[2][2]) * s64(v.z));

    SetMacAndIR<1>(x, shift, lm);
    SetMacAndIR<2>(y, shift, lm);
    SetMacAndIR<3>(z, shift, lm);
}

#undef CMASE1
#undef CMASE2
#undef CMASE3

s64 GTE::RTPKernel(const Vector3<s16>& v, u8 shift, bool lm) {
    MatrixMultResult rtp_vec = MatrixMultiply(rot_matrix, v, tl_vec);

    // TODO: should lm bit be ignored for IR saturation?

    SetMacAndIR<1>(rtp_vec.x, shift, lm);
    SetMacAndIR<2>(rtp_vec.y, shift, lm);
    SetMac<3>(rtp_vec.z, shift);

    // IR3 saturation flag triggers if 'MAC3 SAR 12' exceeds -8000h..+7FFFh, regardless of sf value
    SaturateIR<3>(s32(rtp_vec.z) >> 12, lm); // ignore the saturated output
    // while the actual IR3 register is saturated if 'MAC3' exceeds -8000h..+7FFFh
    ir_vec.ir3 = static_cast<s16>(std::clamp<s32>(mac_vec.mac3, lm ? 0 : -0x8000, 0x7FFF));

    return rtp_vec.z;
}

u32 GTE::UNRDivide(u32 lhs, u32 rhs) {
    // table from https://problemkaputt.de/psx-spx.htm#gtedivisioninaccuracy
    static const std::array<u8, 257> UNRTable {
        0xFF,0xFD,0xFB,0xF9,0xF7,0xF5,0xF3,0xF1,0xEF,0xEE,0xEC,0xEA,0xE8,0xE6,0xE4,0xE3,
        0xE1,0xDF,0xDD,0xDC,0xDA,0xD8,0xD6,0xD5,0xD3,0xD1,0xD0,0xCE,0xCD,0xCB,0xC9,0xC8,
        0xC6,0xC5,0xC3,0xC1,0xC0,0xBE,0xBD,0xBB,0xBA,0xB8,0xB7,0xB5,0xB4,0xB2,0xB1,0xB0,
        0xAE,0xAD,0xAB,0xAA,0xA9,0xA7,0xA6,0xA4,0xA3,0xA2,0xA0,0x9F,0x9E,0x9C,0x9B,0x9A, // 0x00..0x3F
        0x99,0x97,0x96,0x95,0x94,0x92,0x91,0x90,0x8F,0x8D,0x8C,0x8B,0x8A,0x89,0x87,0x86,
        0x85,0x84,0x83,0x82,0x81,0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x75,0x74,
        0x73,0x72,0x71,0x70,0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,
        0x63,0x62,0x61,0x60,0x5F,0x5E,0x5D,0x5D,0x5C,0x5B,0x5A,0x59,0x58,0x57,0x56,0x55, // 0x40..0x7F
        0x54,0x53,0x53,0x52,0x51,0x50,0x4F,0x4E,0x4D,0x4D,0x4C,0x4B,0x4A,0x49,0x48,0x48,
        0x47,0x46,0x45,0x44,0x43,0x43,0x42,0x41,0x40,0x3F,0x3F,0x3E,0x3D,0x3C,0x3C,0x3B,
        0x3A,0x39,0x39,0x38,0x37,0x36,0x36,0x35,0x34,0x33,0x33,0x32,0x31,0x31,0x30,0x2F,
        0x2E,0x2E,0x2D,0x2C,0x2C,0x2B,0x2A,0x2A,0x29,0x28,0x28,0x27,0x26,0x26,0x25,0x24, // 0x80..0xBF
        0x24,0x23,0x22,0x22,0x21,0x20,0x20,0x1F,0x1E,0x1E,0x1D,0x1D,0x1C,0x1B,0x1B,0x1A,
        0x19,0x19,0x18,0x18,0x17,0x16,0x16,0x15,0x15,0x14,0x14,0x13,0x12,0x12,0x11,0x11,
        0x10,0x0F,0x0F,0x0E,0x0E,0x0D,0x0D,0x0C,0x0C,0x0B,0x0A,0x0A,0x09,0x09,0x08,0x08,
        0x07,0x07,0x06,0x06,0x05,0x05,0x04,0x04,0x03,0x03,0x02,0x02,0x01,0x01,0x00,0x00, // 0xC0..0xFF
        0x00 // < --one extra table entry( for "(d-7FC0h)/80h" = 100h ); -100h
    };

    u32 result = 0;
    if (lhs < rhs * 2) {
        const u32 z = std::countl_zero(static_cast<u16>(rhs));

        const u32 n = lhs << z;
        u32 d = rhs << z;
        const u32 index = (d - 0x7FC0) >> 7;

        const u32 u = UNRTable[index] + 0x101;
        d = static_cast<u32>((0x2000080 - u64(d) * u64(u)) >> 8);
        d = static_cast<u32>((0x80 + u64(d) * u64(u)) >> 8);
        result = std::min<u32>(0x1FFFF, static_cast<u32>((u64(n) * u64(d) + 0x8000) >> 16));
    } else {
        error_flags.div_overflow = 1;
        result = 0x1FFFF;
    }

    return result;
}

void GTE::Reset() {
    error_flags.bits = 0;

    vec0 = {.x = 0, .y = 0, .z = 0};
    vec1 = {.x = 0, .y = 0, .z = 0};
    vec2 = {.x = 0, .y = 0, .z = 0};

    otz = 0;

    rgbc = {.r = 0, .g = 0, .b = 0, .t = 0};
    for (auto& c: rgb) c = {.r = 0, .g = 0, .b = 0, .t = 0};

    unused_reg = 0;

    mac0 = 0;
    mac_vec = {.mac1 = 0, .mac2 = 0, .mac3 = 0};

    ir0 = 0;
    ir_vec = {.x = 0, .y = 0, .z = 0};

    for (auto& s: screen) s = {.x = 0, .y = 0};
    for (auto& s: screen_z) s = 0;

    leading_bit_source = 0;

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
