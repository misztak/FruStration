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
        BitField<u32, u32, 18, 1> otz_saturated;
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
        BitField<u32, bool, 31, 1> master_error;

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

    struct ScreenPointXY {
        s16 x, y;
    };

    struct MatrixMultResult {
        s64 x, y, z;
    };

    ALWAYS_INLINE void ResetErrorFlag() {
        error_flags.bits = 0;
    }

    ALWAYS_INLINE void UpdateErrMasterFlag() {
        error_flags.master_error = bool(error_flags.bits & 0x7F87E000);
    }

    [[nodiscard]] u32 CountLeadingBits() const;

    template<u32 ir_id>
    s32 SaturateIR(s32 value, bool lm);

    template<u32 color_id>
    s32 SaturateColor(s32 value);

    template<u32 coord_id>
    s32 SaturateScreenCoordsXY(s32 value);

    s32 SaturateScreenCoordsZ(s32 value);

    template<u32 mac_id>
    void CheckMacOverflow(s64 value);

    template<u32 mac_id>
    s64 CheckMacAndSignExtend(s64 value);

    template<u32 ir_id>
    void SetIR(s32 value, bool lm);

    s64 SetMac0(s64 value);

    template<u32 mac_id>
    s64 SetMac(s64 value, u8 shift);

    template<u32 id>
    void SetMacAndIR(s64 value, u8 shift, bool lm);

    void PushScreenX(s32 sz);
    void PushScreenY(s32 sz);
    void PushScreenZ(s32 sz);

    void PushColor(s32 r, s32 g, s32 b);
    void PushColorFromMac();

    void SetOrderTableZ(s64 value);

    void RTPSingle(const Vector3<s16>& v, u8 shift, bool lm, bool last_vertex);
    void NCLIP();
    void MatrixVectorMultiplication(GTE::Command cmd);
    void NCCS(u8 shift, bool lm);
    void NCS(u8 shift, bool lm);
    void NCT(u8 shift, bool lm);
    void SQR(u8 shift, bool lm);
    void AVSZ3();
    void AVSZ4();
    void RTPTriple(u8 shift, bool lm);
    void GPF(u8 shift, bool lm);
    void GPL(u8 shift, bool lm);
    void NCCT(u8 shift, bool lm);

    template<u32 type>
    void NC(const Vector3<s16>& v, u8 shift, bool lm);

    MatrixMultResult MatrixMultiply(const Matrix3x3& m, const Vector3<s16>& v);
    MatrixMultResult MatrixMultiply(const Matrix3x3& m, const Vector3<s16>& v, const Vector3<s32>& t);
    void MVMVAKernelBugged(const Matrix3x3& m, const Vector3<s16>& v, const Vector3<s32>& t, u8 shift, bool lm);
    s64 RTPKernel(const Vector3<s16>& v, u8 shift, bool lm);

    u32 UNRDivide(u32 lhs, u32 rhs);

    //u8 m_shift = 0;
    //bool m_lm = 0;

    // vectors 0, 1 and 2
    Vector3<s16> vec0 {};
    Vector3<s16> vec1 {};
    Vector3<s16> vec2 {};

    Color32 rgbc {};
    Color32 rgb[3] {};

    // unused r/w register
    u32 unused_reg = 0;

    s32 leading_bit_source = 0;

    // average z value
    u16 otz = 0;

    // screen points xy (3 stage FIFO)
    ScreenPointXY screen[3] {};

    // screen points z (4 stage FIFO)
    u16 screen_z[4] {};

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