#include "bus.h"

#include <fstream>

#include "imgui.h"
#include "imgui_memory_editor.h"

#include "cdrom.h"
#include "common/asserts.h"
#include "common/config.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "debugger/debugger.h"
#include "dma.h"
#include "gpu.h"
#include "interrupt.h"
#include "system.h"
#include "timer/timers.h"

LOG_CHANNEL(BUS);

BUS::BUS(System* system) : sys(system), bios(BIOS_SIZE), ram(RAM_SIZE, 0xCA), scratchpad(SCRATCH_SIZE) {}

bool BUS::LoadBIOS() {
    std::string path = Config::bios_path.Get();
    LogInfo("Loading BIOS from file {}", path);

    std::ifstream file(path, std::ifstream::binary);
    if (!file || !file.good()) {
        LogWarn("Failed to open BIOS file {}", path);
        return false;
    }
    file.seekg(0, std::ifstream::end);
    long length = file.tellg();
    file.seekg(0, std::ifstream::beg);

    if (length != BIOS_SIZE) {
        LogWarn("BIOS file {} has invalid length {}", path, length);
        return false;
    }

    file.read(reinterpret_cast<char*>(bios.data()), BIOS_SIZE);
    return true;
}

bool BUS::LoadPsExe() {
    // Called at the injection address (0x80030000) during BIOS setup
    // Do not load the psexe if:
    //  1. no psexe file was specified
    //  2. a game file was specified (regardless of whether a psexe file was specified)
    //  3. the psexe file is invalid/malformed
    // If one of the conditions is met the emulator resumes normal BIOS setup
    if (Config::psexe_file_path.empty() || !Config::ps_bin_file_path.empty()) return false;

    std::string path = Config::psexe_file_path;
    LogInfo("Loading PS-EXE from file {}", path);

    std::ifstream file(path, std::ifstream::binary);
    if (!file || !file.good()) {
        LogWarn("Failed to open PS-EXE file {}", path);
        return false;
    }
    file.seekg(0, std::ifstream::end);
    long length = file.tellg();
    file.seekg(0, std::ifstream::beg);

    if (length <= PSEXE_HEADER_SIZE) {
        LogWarn("PS-EXE: invalid file size ({} byte)", length);
        return false;
    }

    std::vector<u8> buffer(length);
    file.read(reinterpret_cast<char*>(buffer.data()), length);

    std::string magic(buffer.begin(), buffer.begin() + 8);
    if (magic != "PS-X EXE") {
        LogWarn("PS-EXE: Invalid header {}", magic);
        return false;
    }

    // entry point for psexe execution
    u32 execution_start_addr = *reinterpret_cast<u32*>(buffer.data() + 0x10);

    // physical start address of TEXT segment (always the first segment in a psexe file)
    u32 text_segment_start = *reinterpret_cast<u32*>(buffer.data() + 0x18) & 0x1FFFFFFF;
    u32 text_segment_size = *reinterpret_cast<u32*>(buffer.data() + 0x1C);
    Assert(text_segment_start + length - PSEXE_HEADER_SIZE < RAM_SIZE);

    std::memcpy(ram.data() + text_segment_start, buffer.data() + PSEXE_HEADER_SIZE, length - PSEXE_HEADER_SIZE);

    sys->cpu->sp.pc = execution_start_addr;
    sys->cpu->next_pc = execution_start_addr + 4;
    sys->cpu->instr.value = Load<u32>(execution_start_addr);

    // new stack pointer value (ignore if 0)
    u32 stack_start_addr = *reinterpret_cast<u32*>(buffer.data() + 0x30);
    if (stack_start_addr != 0) sys->cpu->gp.sp = stack_start_addr;

    LogInfo("PSEXE file injected at:");
    LogInfo("    pc=0x{:08x}", execution_start_addr);
    LogInfo("    text=[0x{:08x}, 0x{:x}]", text_segment_start, text_segment_size);
    LogInfo("    sp=0x{:08x}", sys->cpu->gp.sp);

    return true;
}

static constexpr bool InArea(u32 start, u32 length, u32 address) {
    return address >= start && address < start + length;
}

template<typename ValueType>
ValueType BUS::Load(u32 address) {
    static_assert(std::is_same<ValueType, u32>::value || std::is_same<ValueType, u16>::value ||
                  std::is_same<ValueType, u8>::value);

#ifdef USE_WATCHPOINTS
    if (sys->debugger->IsWatchpoint(address) && sys->debugger->WatchpointEnabledOnLoad(address)) {
        LOG_DEBUG << "Hit load watchpoint";
        sys->cpu->halt = true;
    }
#endif

    const u32 masked_addr = MaskRegion(address);

    // RAM
    if (InArea(RAM_START, RAM_SIZE, masked_addr)) return *reinterpret_cast<ValueType*>(ram.data() + masked_addr);
    // Scratchpad
    if (InArea(SCRATCH_START, SCRATCH_SIZE, masked_addr))
        return *reinterpret_cast<ValueType*>(scratchpad.data() + (masked_addr - SCRATCH_START));
    // IO Ports
    if (InArea(IO_PORTS_START, IO_PORTS_SIZE, masked_addr)) {
        switch (masked_addr) {
            case (0x1F801070): return sys->interrupt->LoadStat();
            case (0x1F801074): return sys->interrupt->LoadMask();
            case (0x1F801810): return sys->gpu->gpu_read;                 // GPUREAD
            case (0x1F801814): return (ValueType)sys->gpu->ReadStat();    // GPUSTAT
        }

        // DMA
        if (InArea(0x1F801080, 120, masked_addr)) return (ValueType)sys->dma->Load(masked_addr - 0x1F801080);

        // Timer
        if (InArea(0x1F801100, 48, masked_addr)) return (ValueType)sys->timers->Load(masked_addr - 0x1F801100);

        // CDROM
        if (InArea(0x1F801800, 4, masked_addr)) return (ValueType)sys->cdrom->Load(masked_addr - 0x1F801800);

        if (InArea(0x1F801C00, 644, masked_addr)) return 0;    // SPU

        // Joypad
        if (InArea(0x1F801040, 16, masked_addr)) {
            //LOG_DEBUG << "Load from Joypad [Unimplemented]";
            return 0b11;
        }

        Panic("Tried to load from IO Ports [0x{:08X}]", address);
    }
    // BIOS
    if (InArea(BIOS_START, BIOS_SIZE, masked_addr))
        return *reinterpret_cast<ValueType*>(bios.data() + (masked_addr - BIOS_START));
    // Cache Control
    if (InArea(CACHE_CTRL_START, CACHE_CTRL_SIZE, masked_addr))
        Panic("Tried to load from Cache Control [0x{:08X}]", address);
    // Expansion Region 1
    if (InArea(EXP_REG_1_START, EXP_REG_1_SIZE, masked_addr)) {
        LogWarn("Tried to load from Expansion Region 1 [0x{:08X}]", address);
        return 0xFF;
    }
    // Expansion Region 2
    if (InArea(EXP_REG_2_START, EXP_REG_2_SIZE, masked_addr)) {
        Panic("Tried to load from Expansion Region 2 [0x{:08X}]", address);
    }
    // Expansion Region 3
    if (InArea(EXP_REG_3_START, EXP_REG_3_SIZE, masked_addr)) {
        Panic("Tried to load from Expansion Region 3 [0x{:08X}]", address);
    }

    Panic("Tried to load from invalid address [0x{:08X}]", address);
    return 0;
}

template<typename Value>
void BUS::Store(u32 address, Value value) {
    static_assert(std::is_same<decltype(value), u32>::value || std::is_same<decltype(value), u16>::value ||
                  std::is_same<decltype(value), u8>::value);

#ifdef USE_WATCHPOINTS
    if (sys->debugger->IsWatchpoint(address) && sys->debugger->WatchpointEnabledOnStore(address)) {
        LOG_DEBUG << "Hit store watchpoint";
        sys->cpu->halt = true;
    }
#endif

    const u32 masked_addr = MaskRegion(address);

    // RAM
    if (InArea(RAM_START, RAM_SIZE, masked_addr)) {
        std::copy(reinterpret_cast<u8*>(&value), reinterpret_cast<u8*>(&value) + sizeof(value),
                  ram.data() + masked_addr);
        return;
    }
    // Scratchpad
    if (InArea(SCRATCH_START, SCRATCH_SIZE, masked_addr)) {
        std::copy(reinterpret_cast<u8*>(&value), reinterpret_cast<u8*>(&value) + sizeof(value),
                  scratchpad.data() + (masked_addr - SCRATCH_START));
        return;
    }
    // IO Ports
    if (InArea(IO_PORTS_START, IO_PORTS_SIZE, masked_addr)) {
        switch (masked_addr) {
            case (0x1F801070): sys->interrupt->StoreStat(value); return;
            case (0x1F801074): sys->interrupt->StoreMask(value); return;
            case (0x1F801810): sys->gpu->SendGP0Cmd(value); return;
            case (0x1F801814): sys->gpu->SendGP1Cmd(value); return;
        }

        // DMA
        if (InArea(0x1F801080, 120, masked_addr)) {
            sys->dma->Store(masked_addr - 0x1F801080, value);
            return;
        }

        // Timer
        if (InArea(0x1F801100, 48, masked_addr)) {
            sys->timers->Store(masked_addr - 0x1F801100, value);
            return;
        }

        if (InArea(0x1F801C00, 644, masked_addr)) return;    // SPU

        // CDROM
        if (InArea(0x1F801800, 4, masked_addr)) {
            sys->cdrom->Store(masked_addr - 0x1F801800, value);
            return;
        }

        // ignore it
        return;
    }
    // BIOS
    if (InArea(BIOS_START, BIOS_SIZE, masked_addr)) {
        LogWarn("Tried to store value in BIOS address range [0x{:08X}] - Ignored", address);
        return;
    }
    // Cache Control
    if (InArea(CACHE_CTRL_START, CACHE_CTRL_SIZE, masked_addr)) {
        LogWarn("Store call to Cache Control [0x{:X} @ 0x{:08X}] - Ignored", value, address);
        return;
    }
    // Expansion Region 1
    if (InArea(EXP_REG_1_START, EXP_REG_1_SIZE, masked_addr)) {
        Panic("Tried to store in Expansion Region 1 [0x{:08X}]", address);
    }
    // Expansion Region 2
    if (InArea(EXP_REG_2_START, EXP_REG_2_SIZE, masked_addr)) {
        LogWarn("Tried to store in Expansion Region 2 [0x{:X} @ 0x{:08X}] - Ignored", value, address);
        return;
    }
    // Expansion Region 3
    if (InArea(EXP_REG_3_START, EXP_REG_3_SIZE, masked_addr)) {
        Panic("Tried to store in Expansion Region 3 [0x{:08X}]", address);
    }

    Panic("Tried to store in invalid address [0x{:08X}]", address);
}

u8 BUS::Peek(u32 address) {
    // TODO: make Peek a template function like Load and Store

    const auto ToU8 = [&](u32 value) { return (value >> (address & 0x3)) & 0xFF; };

    const u32 physical_addr = MaskRegion(address);

    // RAM
    if (InArea(RAM_START, RAM_SIZE, physical_addr)) return *(ram.data() + physical_addr);
    // Scratchpad
    if (InArea(SCRATCH_START, SCRATCH_SIZE, physical_addr))
        return *(scratchpad.data() + (physical_addr - SCRATCH_START));
    // MMIO
    if (InArea(IO_PORTS_START, IO_PORTS_SIZE, physical_addr)) {
        if (InArea(0x1F801070, 4, physical_addr)) return ToU8(sys->interrupt->LoadStat());
        if (InArea(0x1F801074, 4, physical_addr)) return ToU8(sys->interrupt->LoadMask());
        if (InArea(0x1F801810, 4, physical_addr)) return ToU8(sys->gpu->gpu_read);
        if (InArea(0x1F801814, 4, physical_addr)) return ToU8(sys->gpu->ReadStat());

        // DMA
        if (InArea(0x1F801080, 120, physical_addr)) return sys->dma->Peek(physical_addr);

        // Timer
        if (InArea(0x1F801100, 48, physical_addr)) return sys->timers->Peek(physical_addr);

        // CDROM
        if (InArea(0x1F801800, 4, physical_addr)) return sys->cdrom->Peek(physical_addr);

        if (InArea(0x1F801C00, 644, physical_addr)) return 0;    // SPU
        if (InArea(0x1F801040, 16, physical_addr)) return 0;     // Joypad
    }
    // BIOS
    if (InArea(BIOS_START, BIOS_SIZE, physical_addr)) return *(bios.data() + (physical_addr - BIOS_START));
    // Cache Control
    if (InArea(CACHE_CTRL_START, CACHE_CTRL_SIZE, physical_addr)) return 0;
    // Expansion Region 1
    if (InArea(EXP_REG_1_START, EXP_REG_1_SIZE, physical_addr)) return 0xFF;
    // Expansion Region 2
    if (InArea(EXP_REG_2_START, EXP_REG_2_SIZE, physical_addr)) return 0xFF;
    // Expansion Region 3
    if (InArea(EXP_REG_3_START, EXP_REG_3_SIZE, physical_addr)) return 0xFF;

    return 0;
}

u32 BUS::Peek32(u32 address) {
    // HACK: just glue the values together until BUS::Peek supports all data sizes
    u32 result = 0;
    result |= Peek(address);
    result |= Peek(address + 1) << 8;
    result |= Peek(address + 2) << 16;
    result |= Peek(address + 3) << 24;

    return result;
}

void BUS::Reset() {
    // TODO: add ability to change bios file during reset
    std::fill(ram.begin(), ram.end(), 0xCA);
}

void BUS::DrawMemEditor(bool* open) {
    // probably not very useful
    static MemoryEditor mem_editor;
    ImGui::Begin("Memory Editor", open);

    if (ImGui::BeginTabBar("__mem_editor_tabs")) {
        if (ImGui::BeginTabItem("RAM (2 MiB)")) {
            mem_editor.DrawContents(ram.data(), ram.size(), 0);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BIOS (512 KiB)")) {
            mem_editor.DrawContents(bios.data(), bios.size(), 0);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("VRAM (1 MiB)")) {
            mem_editor.DrawContents((u8*)sys->gpu->GetVRAM(), GPU::VRAM_SIZE * 2, 0);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

template u32 BUS::Load<u32>(u32 address);
template u16 BUS::Load<u16>(u32 address);
template u8 BUS::Load<u8>(u32 address);

template void BUS::Store<u32>(u32 address, u32 value);
template void BUS::Store<u16>(u32 address, u16 value);
template void BUS::Store<u8>(u32 address, u8 value);
