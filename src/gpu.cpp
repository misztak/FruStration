#include "gpu.h"

GPU::GPU() {
    status.display_disabled = true;
    // pretend that everything is ok
    status.can_receive_cmd_word = true;
    status.can_send_vram_to_cpu = true;
    status.can_receive_dma_block = true;
}
void GPU::Init() {}

u32 GPU::ReadStat() { return status.value; }
