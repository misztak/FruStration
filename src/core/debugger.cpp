#include "debugger.h"

#include "cpu.h"
#include "imgui.h"

void Debugger::Init(CPU::CPU* c) {
    cpu = c;
}

void Debugger::DrawDebugger(bool* open) {
    ImGui::Begin("Debugger", open);
    ImGui::Checkbox("Active", &attached);
    ImGui::Separator();

    ImGui::PushID(1);
    ImGui::Text("Breakpoints");
    bool add_bp = ImGui::Button("Add");
    ImGui::SameLine();
    static u32 bp_address = 0u;
    ImGui::InputScalar("", ImGuiDataType_U32, &bp_address, nullptr,
                       nullptr, "%08X",ImGuiInputTextFlags_CharsHexadecimal);
    if (add_bp) {
        breakpoints.insert_or_assign(bp_address, Breakpoint());
    }
    if (!breakpoints.empty()) {
        for (auto& entry : breakpoints) {
            ImGui::Text("Breakpoint @ 0x%08X", entry.first);
        }
    }
    ImGui::PopID();
    ImGui::Separator();

    ImGui::PushID(2);
    ImGui::Text("Watchpoints");
    bool add_wp = ImGui::Button("Add");
    ImGui::SameLine();
    static u32 wp_address = 0u;
    ImGui::InputScalar("", ImGuiDataType_U32, &wp_address, nullptr,
                       nullptr, "%08X",ImGuiInputTextFlags_CharsHexadecimal);
    static bool on_read = true, on_write = true;
    ImGui::SameLine(); ImGui::Checkbox("On Load", &on_read);
    ImGui::SameLine(); ImGui::Checkbox("On Store", &on_write);
    Watchpoint::Type wp_type =
        (on_read && on_write)
            ? Watchpoint::ENABLED
            : (on_read ? Watchpoint::ONLY_LOAD : (on_write ? Watchpoint::ONLY_STORE : Watchpoint::DISABLED));
    if (add_wp) {
        watchpoints.insert_or_assign(wp_address, Watchpoint(wp_type));
    }
    if (!watchpoints.empty()) {
        for (auto& entry : watchpoints) {
            ImGui::Text("Watchpoint [%s] @ 0x%08X", entry.second.TypeToString(), entry.first);
        }
    }
    ImGui::PopID();
    ImGui::Separator();

    if (attached) {
        u32 start = (ring_ptr + 1) & 127;
        for (u32 i = start; i < start + 128; i++) {
            auto& instr = last_instructions[i & 127];
            if (instr.first == 0) continue;
            ImGui::BeginGroup();
            ImGui::TextUnformatted(cpu->disassembler.InstructionAt(instr.first, instr.second, false).c_str());
            ImGui::EndGroup();
        }
    }

    ImGui::End();
}

void Debugger::Reset() {
    // we only want to reset the instruction buffer
    ring_ptr = 0;
    for (auto& e : last_instructions) {
        e.first = 0;
        e.second = 0;
    }
}
