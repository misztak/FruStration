#include "bus.h"

#include <fstream>

#include "imgui.h"
#include "imgui_memory_editor.h"
#include "fmt/format.h"

#include "cdrom.h"
#include "cpu.h"
#include "debug_utils.h"
#include "debugger.h"
#include "dma.h"
#include "gpu.h"
#include "interrupt.h"
#include "system.h"
#include "timers.h"

LOG_CHANNEL(BUS);

BUS::BUS(System* system) : sys(system), bios(BIOS_SIZE), ram(RAM_SIZE, 0xCA), scratchpad(SCRATCH_SIZE) {}

bool BUS::LoadBIOS(const std::string& path) {
    LOG_INFO << "Loading BIOS from file " << path;

    std::ifstream file(path, std::ifstream::binary);
    if (!file || !file.good()) {
        LOG_WARN << "Failed to open BIOS file " << path;
        return false;
    }
    file.seekg(0, std::ifstream::end);
    long length = file.tellg();
    file.seekg(0, std::ifstream::beg);

    if (length != BIOS_SIZE) {
        LOG_WARN << "BIOS file" << path << "has invalid length " << length;
        return false;
    }

    file.read(reinterpret_cast<char*>(bios.data()), BIOS_SIZE);
    return true;
}

bool BUS::LoadPsExe(const std::string& path) {
    LOG_INFO << "Loading PS-EXE from file " << path;

    std::ifstream file(path, std::ifstream::binary);
    if (!file || !file.good()) {
        LOG_WARN << "Failed to open PS-EXE file " << path;
        return false;
    }
    file.seekg(0, std::ifstream::end);
    long length = file.tellg();
    file.seekg(0, std::ifstream::beg);

    if (length < 0x800 || length > RAM_SIZE) {
        LOG_WARN << "PS-EXE: invalid file size (" << length << " byte)";
        return false;
    }

    std::vector<u8> buffer(length);
    file.read(reinterpret_cast<char*>(buffer.data()), length);

    std::string magic(buffer.begin(), buffer.begin() + 8);
    if (magic != "PS-X EXE") {
        LOG_WARN << "PS-EXE: Invalid header " << magic;
        return false;
    }

    u32 exec_data = *reinterpret_cast<u32*>(buffer.data() + 0x10);
    u32 text_start = *reinterpret_cast<u32*>(buffer.data() + 0x18);
    // u32 text_size = *reinterpret_cast<u32*>(buffer.data() + 0x1C);
    // u32 stack_start_address = *reinterpret_cast<u32*>(buffer.data() + 0x30);

    for (u32 i = 0x800; i < length; i++) {
        ram.at(exec_data & 0x1FFFFFFF) = buffer[i];
        exec_data++;
    }

    sys->cpu->sp.pc = text_start;
    sys->cpu->next_pc = text_start;
    sys->cpu->instr.value = Load<u32>(text_start);

    return true;
}

static constexpr bool InArea(u32 start, u32 length, u32 address) {
    return address >= start && address < start + length;
}

template <typename ValueType>
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
            case(0x1F801070): return sys->interrupt->LoadStat();
            case(0x1F801074): return sys->interrupt->LoadMask();
            case(0x1F801810): return sys->gpu->gpu_read; // GPUREAD
            case(0x1F801814): return (ValueType) sys->gpu->ReadStat(); // GPUSTAT
        }

        // DMA
        if (InArea(0x1F801080, 120, masked_addr))
            return (ValueType) sys->dma->Load(masked_addr - 0x1F801080);

        // Timer
        if (InArea(0x1F801100, 48, masked_addr))
            return (ValueType) sys->timers->Load(masked_addr - 0x1F801100);

        // CDROM
        if (InArea(0x1F801800, 4, masked_addr))
            return (ValueType) sys->cdrom->Load(masked_addr - 0x1F801800);


        if (InArea(0x1F801C00, 644, masked_addr)) return 0; // SPU

        // Joypad
        if (InArea(0x1F801040, 16, masked_addr)) {
            //LOG_DEBUG << "Load from Joypad [Unimplemented]";
            return 0b11;
        }

        Panic("Tried to load from IO Ports [0x%08X]", address);
    }
    // BIOS
    if (InArea(BIOS_START, BIOS_SIZE, masked_addr))
        return *reinterpret_cast<ValueType*>(bios.data() + (masked_addr - BIOS_START));
    // Cache Control
    if (InArea(CACHE_CTRL_START, CACHE_CTRL_SIZE, masked_addr)) Panic("Tried to load from Cache Control [0x%08X]", address);
    // Expansion Region 1
    if (InArea(EXP_REG_1_START, EXP_REG_1_SIZE, masked_addr)) {
        LOG_WARN << fmt::format("Tried to load from Expansion Region 1 [0x{:08X}]", address);
        return 0xFF;
    }
    // Expansion Region 2
    if (InArea(EXP_REG_2_START, EXP_REG_2_SIZE, masked_addr)) {
        Panic("Tried to load from Expansion Region 2 [0x%08X]", address);
    }
    // Expansion Region 3
    if (InArea(EXP_REG_3_START, EXP_REG_3_SIZE, masked_addr)) {
        Panic("Tried to load from Expansion Region 3 [0x%08X]", address);
    }

    Panic("Tried to load from invalid address [0x%08X]", address);
    return 0;
}

template <typename Value>
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
            case(0x1F801070): sys->interrupt->StoreStat(value); return;
            case(0x1F801074): sys->interrupt->StoreMask(value); return;
            case(0x1F801810): sys->gpu->SendGP0Cmd(value); return;
            case(0x1F801814): sys->gpu->SendGP1Cmd(value); return;
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

        if (InArea(0x1F801C00, 644, masked_addr)) return; // SPU

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
        LOG_WARN << fmt::format("Tried to store value in BIOS address range [0x{:08X}] - Ignored", address);
        return;
    }
    // Cache Control
    if (InArea(CACHE_CTRL_START, CACHE_CTRL_SIZE, masked_addr)) {
        LOG_WARN << fmt::format("Store call to Cache Control [0x{:X} @ 0x{:08X}] - Ignored", value, address);
        return;
    }
    // Expansion Region 1
    if (InArea(EXP_REG_1_START, EXP_REG_1_SIZE, masked_addr)) {
        Panic("Tried to store in Expansion Region 1 [0x%08X]", address);
    }
    // Expansion Region 2
    if (InArea(EXP_REG_2_START, EXP_REG_2_SIZE, masked_addr)) {
        LOG_WARN << fmt::format("Tried to store in Expansion Region 2 [0x{:X} @ 0x{:08X}] - Ignored", value, address);
        return;
    }
    // Expansion Region 3
    if (InArea(EXP_REG_3_START, EXP_REG_3_SIZE, masked_addr)) {
        Panic("Tried to store in Expansion Region 3 [0x%08X]", address);
    }

    Panic("Tried to store in invalid address [0x%08X]", address);
    return;
}

u8 BUS::Peek(u32 address) {
    const auto ToU8 = [&](u32 value) {
        return (value >> (address & 0x3)) & 0xFF;
    };

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
        if (InArea(0x1F801080, 120, physical_addr))
            return sys->dma->Peek(physical_addr);

        // Timer
        if (InArea(0x1F801100, 48, physical_addr))
            return sys->timers->Peek(physical_addr);

        // CDROM
        if (InArea(0x1F801800, 4, physical_addr))
            return sys->cdrom->Peek(physical_addr);


        if (InArea(0x1F801C00, 644, physical_addr)) return 0; // SPU
        if (InArea(0x1F801040, 16, physical_addr)) return 0;  // Joypad
    }
    // BIOS
    if (InArea(BIOS_START, BIOS_SIZE, physical_addr))
        return *(bios.data() + (physical_addr - BIOS_START));
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

void BUS::Reset() {
    // TODO: add ability to change bios file during reset
    std::fill(ram.begin(), ram.end(), 0xCA);
}

void BUS::DrawMemEditor(bool* open) {
    // probably not very useful
    static MemoryEditor mem_editor;
    ImGui::Begin("Memory Editor", open);

    if (ImGui::BeginTabBar("__mem_editor_tabs")) {
        if (ImGui::BeginTabItem("RAM")) {
            mem_editor.DrawContents(ram.data(), ram.size(), 0);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BIOS")) {
            mem_editor.DrawContents(bios.data(), bios.size(), 0);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("VRAM")) {
            mem_editor.DrawContents(sys->gpu->GetVRAM(), GPU::VRAM_SIZE * 2, 0);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void BUS::DumpRAM(const std::string& path) {
    std::ofstream out_file(path);
    out_file.write((char*) ram.data(), ram.size());
    LOG_INFO << "Dumped RAM into file " << path;
}

template u32 BUS::Load<u32>(u32 address);
template u16 BUS::Load<u16>(u32 address);
template u8 BUS::Load<u8>(u32 address);

template void BUS::Store<u32>(u32 address, u32 value);
template void BUS::Store<u16>(u32 address, u16 value);
template void BUS::Store<u8>(u32 address, u8 value);
