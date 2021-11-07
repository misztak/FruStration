#include "debugger.h"

#include "imgui.h"

#include "debug_utils.h"
#include "system.h"
#include "cpu.h"
#include "bus.h"

LOG_CHANNEL(Debugger);

Debugger::Debugger(System* system): sys(system) {}

void Debugger::AddBreakpoint(u32 address) {
    breakpoints.insert_or_assign(address, Breakpoint());
}

void Debugger::RemoveBreakpoint(u32 address) {
    breakpoints.erase(address);
}

void Debugger::ToggleBreakpoint(u32 address) {
    auto bp = breakpoints.find(address);
    if (bp != breakpoints.end()) {
        bp->second.enabled = !bp->second.enabled;
    }
}

void Debugger::SetPausedState(bool paused, bool _single_step) {
    sys->cpu->halt = paused;
    single_step = _single_step;
}

void Debugger::DrawDebugger(bool* open) {
    ImGui::Begin("Debugger", open);

    ImGui::Checkbox("Show instructions", &show_disasm_view); ImGui::SameLine();
    ImGui::Checkbox("Frame Step", &single_frame); ImGui::SameLine();
    ImGui::Checkbox("Single Step", &single_step); ImGui::SameLine();

    if (ImGui::Button("Next Frame") || ImGui::IsKeyReleased(63)) {
        if (single_frame) {
            sys->cpu->halt = false;
            LOG_INFO << "Next Frame";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Step") || ImGui::IsKeyReleased(65)) {
        if (single_step) sys->cpu->halt = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Continue") || ImGui::IsKeyReleased(66)) {
        sys->cpu->halt = false;
        single_step = false;
        single_frame = false;
    }
    ImGui::Separator();

    ImGui::PushID("__bp_view");
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
        if (ImGui::TreeNode("__breakpoint_node", "Active")) {
            for (auto& entry : breakpoints) {
                ImGui::PushID(entry.first);
                ImGui::Text("Breakpoint @ 0x%08X", entry.first); ImGui::SameLine();
                if (ImGui::Button("-")) {
                    breakpoints.erase(entry.first);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::PopID();
    ImGui::Separator();

#ifdef USE_WATCHPOINTS

    ImGui::PushID("__wp_view");
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
        if (ImGui::TreeNode("__watchpoint_node", "Active")) {
            for (auto& entry : watchpoints) {
                ImGui::PushID(entry.first);
                ImGui::Text("Watchpoint [%s] @ 0x%08X", entry.second.TypeToString(), entry.first); ImGui::SameLine();
                if (ImGui::Button("-")) {
                    watchpoints.erase(entry.first);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::PopID();
    ImGui::Separator();

#endif

    if (show_disasm_view) {
        static bool locked_to_bottom = true;
        const ImVec4 orange(.8f, .6f, .3f, 1.f);
        const bool is_line_visible = ImGui::BeginChild("__disasm_view", ImVec2(0, 0), true,
                                                       ImGuiWindowFlags_MenuBar);
        u32 start = ring_ptr & BUFFER_MASK;

        if (ImGui::BeginMenuBar()) {
            ImGui::Checkbox("Scroll lock", &locked_to_bottom);
            ImGui::EndMenuBar();
        }

        if (is_line_visible) {
            for (u32 i = start; i < start + BUFFER_SIZE; i++) {
                auto& instr = last_instructions[i & BUFFER_MASK];
                if (instr.first == 0) continue;
                if ((i & BUFFER_MASK) == ((ring_ptr - 1) & BUFFER_MASK)) {
                    ImGui::TextColored(orange, "%s   <---",
                            sys->cpu->disassembler.InstructionAt(instr.first, instr.second, false).c_str());
                } else {
                    ImGui::TextUnformatted(
                        sys->cpu->disassembler.InstructionAt(instr.first, instr.second, false).c_str());
                }
                if (locked_to_bottom) ImGui::SetScrollHere(1.f);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

System* Debugger::GetContext() { return sys; }

void Debugger::Reset() {
    // we only want to reset the instruction buffer
    ring_ptr = 0;
    for (auto& e : last_instructions) {
        e.first = 0;
        e.second = 0;
    }
}
