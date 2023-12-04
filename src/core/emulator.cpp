#include "emulator.h"

#include "bus.h"
#include "common/asserts.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "debugger/gdb_stub.h"
#include "gpu.h"
#include "peripherals.h"
#include "timer/timers.h"

LOG_CHANNEL(Emulator);

bool Emulator::LoadBIOS() {
    return sys.bus->LoadBIOS();
}

bool Emulator::LoadPsExe() {
    return sys.bus->LoadPsExe();
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
    SetPaused(true);

    sys.Reset();
}

std::tuple<u32, u32, bool> Emulator::DisplayInfo() {
    return {sys.gpu->HorizontalRes(), sys.gpu->VerticalRes(), sys.gpu->In24BPPMode()};
}

bool Emulator::IsPaused() {
    return sys.cpu->halt;
}

void Emulator::SetPaused(bool halt) {
    sys.cpu->halt = halt;
    LogInfo("{} execution", halt ? "Pausing" : "Resuming");
}

u8* Emulator::GetVideoOutput() {
    return sys.gpu->GetVideoOutput();
}

u16* Emulator::GetVRAM() {
    return sys.gpu->GetVRAM();
}

Controller& Emulator::GetMainController() {
    return sys.peripherals->GetController1();
}

void Emulator::StartGDBServer() {
    SetPaused(true);

    Assert(sys.debugger.get());
    GDB::Init(Config::gdb_server_port.Get(), sys.debugger.get());
}

void Emulator::HandleGDBClientRequest() {
    if (!Config::gdb_server_enabled.Get()) return;

    GDB::HandleClientRequest();
}

void Emulator::DrawDebugWindows() {
    if (Config::draw_mem_viewer) sys.bus->DrawMemEditor(&Config::draw_mem_viewer);
    if (Config::draw_cpu_state) sys.cpu->DrawCpuState(&Config::draw_cpu_state);
    if (Config::draw_gpu_state) sys.gpu->DrawGpuState(&Config::draw_gpu_state);
    if (Config::draw_debugger) sys.debugger->DrawDebugger(&Config::draw_debugger);
    if (Config::draw_timer_state) sys.timers->DrawTimerState(&Config::draw_timer_state);
}
