#pragma once

#include "gte_types.h"
#include "util/bitfield.h"
#include "util/types.h"

class GTE {
public:
    void Reset();

    void ExecuteCommand(u32 cmd);

    void SetReg(u32 index, u32 value);
    u32 GetReg(u32 index);

private:
    enum class VectorType : u32 {
        V0, V1, V2, IR,
    };

    enum class MatrixType : u32 {
        Rotation,
        Light,
        Color,
        Reserved,
    };

    union {
        BitField<u32, u32, 12, 1> ir0_saturated;
        BitField<u32, u32, 13, 1> sy2_saturated;
        BitField<u32, u32, 14, 1> sx2_saturated;
        BitField<u32, u32, 15, 1> mac0_underflow;
        BitField<u32, u32, 16, 1> mac0_overflow;
        BitField<u32, u32, 17, 1> div_overflow;
        BitField<u32, u32, 18, 1> sz3_or_otz_saturated;
        BitField<u32, u32, 19, 1> fifo_blue_saturated;
        BitField<u32, u32, 20, 1> fifo_green_saturated;
        BitField<u32, u32, 21, 1> fifo_red_saturated;
        BitField<u32, u32, 22, 1> ir3_saturated;
        BitField<u32, u32, 23, 1> ir2_saturated;
        BitField<u32, u32, 24, 1> ir1_saturated;
        BitField<u32, u32, 25, 1> mac3_underflow;
        BitField<u32, u32, 26, 1> mac2_underflow;
        BitField<u32, u32, 27, 1> mac1_underflow;
        BitField<u32, u32, 28, 1> mac3_overflow;
        BitField<u32, u32, 29, 1> mac2_overflow;
        BitField<u32, u32, 30, 1> mac1_overflow;
        BitField<u32, u32, 31, 1> master_error;

        u32 bits = 0;
    } error_flags;

    union Command {
        BitField<u32, u32, 0, 6> real_opcode;
        BitField<u32, bool, 10, 1> lm;
        BitField<u32, u32, 13, 2> mvmva_t_vec;
        BitField<u32, u32, 15, 2> mvmva_m_vec;
        BitField<u32, u32, 17, 2> mvmva_m_mat;
        BitField<u32, bool, 19, 1> sf;
        BitField<u32, u32, 25, 7> opcode;

        u32 value = 0;
    };

    struct Color32 {
        u8 r, g, b, t;
    };

    ALWAYS_INLINE void ResetErrorFlag() {
        error_flags.bits = 0;
    }

    template<u32 ir_id>
    s32 SaturateIR(s32 value, bool lm);

    template<u32 color_id>
    s32 SaturateColor(s32 value);

    template<u32 coord_id>
    s32 SaturateScreenCoords(s32 value);

    template<u32 mac_id>
    void CheckMacOverflow(s64 value);

    template<u32 mac_id>
    s64 CheckMacAndSignExtend(s64 value);

    template<u32 ir_id>
    void SetIR(s32 value, bool lm);

    template<u32 mac_id>
    s64 SetMac(s64 value, u8 shift);

    template<u32 id>
    void SetMacAndIR(s64 value, u8 shift, bool lm);

    void MatrixVectorMultiplication(GTE::Command cmd);

    void MVMVAKernel(const Matrix3x3* m, const Vector3<s16>* v, const Vector3<s32>* t, u8 shift, bool lm);
    void MVMVAKernelBugged(const Matrix3x3* m, const Vector3<s16>* v, const Vector3<s32>* t, u8 shift, bool lm);

    // vectors 0, 1 and 2
    Vector3<s16> vec0 {};
    Vector3<s16> vec1 {};
    Vector3<s16> vec2 {};

    Color32 rgbc {};

    // MAC accumulators
    s32 mac0 = 0;
    Vector3<s32> mac_vec {};

    // IR accumulators
    s16 ir0 = 0;
    Vector3<s16> ir_vec {};

    // rotation matrix
    Matrix3x3 rot_matrix {};

    // translation vector
    Vector3<s32> tl_vec {};

    // light matrix
    Matrix3x3 light_matrix {};

    // background color
    Vector3<s32> background_color {};

    // color matrix
    Matrix3x3 color_matrix {};

    // far color
    Vector3<s32> far_color {};

    // screen offset
    s32 sof_x = 0, sof_y = 0;

    // projection plane distance
    u16 proj_plane_dist = 0;

    // depth queuing parameter A (coeff)
    s16 depth_queue_param_A = 0;

    // depth queuing parameter A (offset)
    s32 depth_queue_param_B = 0;

    // average z scale factors
    s16 z_scale_factor_3 = 0, z_scale_factor_4 = 0;

};