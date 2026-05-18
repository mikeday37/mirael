#include "pch.h"

#include "imgui.h"

#include "ImGuiEx.h"

namespace Mirael::ImGuiEx
{

void RowLabel(const char *labelText, const char *toolTipText)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(labelText);
    if (toolTipText) {
        ImGui::SameLine();
        ToolTipHint(toolTipText);
    }
    ImGui::TableNextColumn();
}

void ToolTipHint(const char *toolTipText)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(toolTipText);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace Mirael::ImGuiEx