#include "debugger.h"

#include "imgui.h"

void Debugger::DrawDebugger(bool* open) {
    ImGui::Begin("Debugger", open);
    ImGui::Text("Sample text");
    ImGui::End();
}
