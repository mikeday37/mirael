#include "pch.h"

#include "imgui.h"

#include "imguiex.h"

namespace Mirael::ImGuiEx
{

void RowLabel(const char *labelText, const char *toolTip)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(labelText);
    if (toolTip) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(toolTip);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    ImGui::TableNextColumn();
}

}; // namespace Mirael::ImGuiEx