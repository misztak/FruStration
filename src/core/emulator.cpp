#include "emulator.h"

#include "bus.h"
#include "common/asserts.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "debugger/gdb_stub.h"
#include "gpu.h"
#include "timer/timers.h"

LOG_CHANNEL(Emulator);

bool Emulator::LoadBIOS(const std::string& bios_path) {
    return sys.bus->LoadBIOS(bios_path);
}

void Emulator::Tick() {
    sys.cpu->Step();
}

bool Emulator::DrawNextFrame() {
    return sys.gpu->draw_frame;
}

void Emulator::ResetDrawFrame() {
    sys.gpu->draw_frame = false;

    // at this point the next frame has been reached
    // stop now if single_frame stepping is enabled
    sys.cpu->halt = sys.debugger->single_frame;
}

void Emulator::Reset() {
    sys.Reset();
}

std::tuple<u32, u32, bool> Emulator::DisplayInfo() {
    bool in_24_bpp_mode = (sys.gpu->ReadStat() & (1u << 21)) != 0;
    return {sys.gpu->HorizontalRes(), sys.gpu->VerticalRes(), in_24_bpp_mode};
}

bool Emulator::IsHalted() {
    return sys.cpu->halt;
}

void Emulator::SetHalt(bool halt) {
    sys.cpu->halt = halt;
    LogInfo("Emulator {} execution", halt ? "paused" : "resumed");
}

u8* Emulator::GetVideoOutput() {
    return sys.gpu->GetVideoOutput();
}

u16* Emulator::GetVRAM() {
    return sys.gpu->GetVRAM();
}

void Emulator::StartGDBServer() {
    if (!cfg_gdb_server_enabled) return;

    SetHalt(true);

    Assert(sys.debugger.get());
    GDB::Init(42069, sys.debugger.get());
}

void Emulator::HandleGDBClientRequest() {
    if (!cfg_gdb_server_enabled) return;

    GDB::HandleClientRequest();
}

void Emulator::DrawDebugWindows() {
    if (draw_mem_viewer) sys.bus->DrawMemEditor(&draw_mem_viewer);
    if (draw_cpu_state) sys.cpu->DrawCpuState(&draw_cpu_state);
    if (draw_gpu_state) sys.gpu->DrawGpuState(&draw_gpu_state);
    if (draw_debugger) sys.debugger->DrawDebugger(&draw_debugger);
    if (draw_timer_state) sys.timers->DrawTimerState(&draw_timer_state);
}
