#pragma once

#include <vector>

#include "types.h"
#include "bitfield.h"
#include "sw_renderer.h"

class TimerController;
class InterruptController;

class GPU {
friend class Renderer;
public:
    static constexpr u32 VRAM_WIDTH = 1024;
    static constexpr u32 VRAM_HEIGHT = 512;
    static constexpr u32 VRAM_SIZE = 1024 * 512;

    GPU();
    void Init(TimerController* timer, InterruptController* icontroller);
    void Reset();

    u32 ReadStat();
    void SendGP0Cmd(u32 cmd);
    void SendGP1Cmd(u32 cmd);

    u16* GetVRAM();
    u8*  GetVideoOutput();

    void DrawGpuState(bool* open);

    bool draw_frame = false;

    u32 gpu_read = 0;

private:
    static constexpr float GPU_CLOCK_RATIO = 11.0f / 7.0f;
    static constexpr float CPU_CLOCK_SPEED = 44100 * 0x300;
    static constexpr float GPU_CLOCK_SPEED = CPU_CLOCK_SPEED * GPU_CLOCK_RATIO;

    static constexpr float REFRESH_RATE_NTSC = 60;
    static constexpr float REFRESH_RATE_PAL = 50;

    ALWAYS_INLINE float RefreshRate() const {
        return status.video_mode == VideoMode::NTSC ? REFRESH_RATE_NTSC : REFRESH_RATE_PAL;
    }

    ALWAYS_INLINE u32 Scanlines() const {
        return status.video_mode == VideoMode::NTSC ? 263 : 314;
    }

    ALWAYS_INLINE u32 VerticalRes() const {
        return status.vertical_res ? 480 : 240;
    }

    u32 HorizontalRes() const {
        if (status.horizontal_res_2) return 368;

        switch (status.horizontal_res_1) {
            case 0: return 256;
            case 1: return 320;
            case 2: return 512;
            case 3: return 640;
        }

        // unreachable
        return 0;
    }

    float DotsPerGpuCycle() const {
        return static_cast<float>(HorizontalRes()) / 2560.f;
    }

    float GpuCyclesPerScanline() const {
        return (GPU_CLOCK_SPEED / RefreshRate()) / Scanlines();
    }

    float DotsPerScanline() const {
        return DotsPerGpuCycle() * GpuCyclesPerScanline();
    }

    ALWAYS_INLINE bool IsGpuInHblank() const {
        return accumulated_dots >= static_cast<float>(HorizontalRes());
    }

    ALWAYS_INLINE bool IsGpuInVblank() const {
        return scanline >= 240;
    }

    std::pair<float, float> GpuCyclesAndDots(u32 cpu_cycles) const {
        const float gpu_cycles = static_cast<float>(cpu_cycles) * GPU_CLOCK_RATIO;
        const float dots = gpu_cycles * DotsPerGpuCycle();
        return std::make_pair(gpu_cycles, dots);
    }

    void StepTmp(u32 cycles);
    u32 CyclesUntilNextEvent();

    void DrawQuadMono();
    void DrawQuadShaded();
    void DrawQuadTextured();

    void DrawTriangleMono();
    void DrawTriangleShaded();
    void DrawTriangleTextured();

    void DrawRectangleMono();
    void DrawRectangleTexture();

    void CopyRectCpuToVram(u32 data = 0);
    void CopyRectVramToCpu();

    void ResetCommand();

    enum class DmaDirection : u32 {
        Off = 0,
        Fifo = 1,
        CpuToGp0 = 2,
        VramToCpu = 3,
    };

    enum class VideoMode : u32 {
        NTSC, PAL
    };

    // TODO: more enums for types
    union GpuStatus {
        u32 value = 0;

        BitField<u32, u32, 0, 4> tex_page_x_base;
        BitField<u32, u32, 4, 1> tex_page_y_base;
        BitField<u32, u32, 5, 2> semi_transparency;
        BitField<u32, u32, 7, 2> tex_page_colors;
        BitField<u32, bool, 9, 1> dither;
        BitField<u32, bool, 10, 1> draw_enable;
        BitField<u32, bool, 11, 1> mask_enable;
        BitField<u32, bool, 12, 1> draw_pixels;
        BitField<u32, bool, 13, 1> interlace_field;
        BitField<u32, bool, 14, 1> reverse;
        BitField<u32, bool, 15, 1> tex_disable;
        BitField<u32, u32, 16, 1> horizontal_res_2;
        BitField<u32, u32, 17, 2> horizontal_res_1;
        BitField<u32, u32, 19, 1> vertical_res;
        BitField<u32, VideoMode, 20, 1> video_mode;
        BitField<u32, u32, 21, 1> display_area_color_depth;
        BitField<u32, bool, 22, 1> vertical_interlace;
        BitField<u32, bool, 23, 1> display_disabled;
        BitField<u32, bool, 24, 1> interrupt_request;
        BitField<u32, bool, 25, 1> dma_data_stat;
        BitField<u32, bool, 26, 1> can_receive_cmd_word;
        BitField<u32, bool, 27, 1> can_send_vram_to_cpu;
        BitField<u32, bool, 28, 1> can_receive_dma_block;
        BitField<u32, DmaDirection, 29, 2> dma_direction;
        BitField<u32, bool, 31, 1> interlace_line_mode;
    } status;

    bool tex_rectangle_xflip = false;
    bool tex_rectangle_yflip = false;

    u8 tex_window_x_mask = 0;
    u8 tex_window_y_mask = 0;
    u8 tex_window_x_offset = 0;
    u8 tex_window_y_offset = 0;

    u16 drawing_area_left = 0;
    u16 drawing_area_top = 0;
    u16 drawing_area_right = 0;
    u16 drawing_area_bottom = 0;

    s16 drawing_x_offset = 0;
    s16 drawing_y_offset = 0;

    u16 display_vram_x_start = 0;
    u16 display_vram_y_start = 0;
    u16 display_horizontal_start = 0;
    u16 display_horizontal_end = 0;
    u16 display_line_start = 0;
    u16 display_line_end = 0;

    enum class Mode : u32 {
        Command,
        DataFromCPU,
        DataToCPU,
    };

    enum class Gp0Command : u32 {
        nop = 0x00,
        clear_cache = 0x01,
        fill_vram = 0x02,
        quad_mono = 0x28,
        quad_textured = 0x2c,
        triangle_shaded = 0x30,
        quad_shaded = 0x38,
        rectangle_textured = 0x64,
        rectangle_mono = 0x68,
        copy_rectangle_cpu_to_vram = 0xa0,
        copy_rectangle_vram_to_cpu = 0xc0,
        draw_mode = 0xe1,
        texture_window = 0xe2,
        draw_area_top_left = 0xe3,
        draw_area_bottom_right = 0xe4,
        draw_offset = 0xe5,
        mask_setting = 0xe6,
    };

    enum class Gp1Command : u32 {
        reset = 0x00,
        cmd_buf_reset = 0x01,
        ack_gpu_interrupt = 0x02,
        display_enable = 0x03,
        dma_dir = 0x04,
        display_vram_start = 0x05,
        display_hor_range = 0x06,
        display_ver_range = 0x07,
        display_mode = 0x08,
        gpu_info = 0x10,
    };

    u32 cycles_until_next_event = 0;

    float accumulated_dots = 0;

    u32 gpu_clock = 0;
    u32 scanline = 0;

    bool was_in_hblank = false, was_in_vblank = false;

    u32 command_counter = 0;
    std::array<u32, 12> command_buffer;

    Mode mode = Mode::Command;
    u32 words_remaining = 0;

    union {
        u32 value = 0;

        BitField<u32, Gp0Command, 24, 8> gp0_op;
        BitField<u32, Gp1Command, 24, 8> gp1_op;
    } command;

    // TODO: is VRAM filled with garbage at boot?
    std::vector<u16> vram;

    // the actual output that gets displayed on the TV
    std::vector<u8> output;

    std::array<Vertex, 4> vertices;
    Rectangle rectangle = {};
    Renderer renderer;

    TimerController* timer_controller = nullptr;
    InterruptController* interrupt_controller = nullptr;
};