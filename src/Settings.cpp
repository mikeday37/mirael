#include "pch.h"

#include "imgui.h"
#include "ine/utilities/style_editor.h"

#include "App.h"
#include "ImGuiEx.h"
#include "Settings.h"

namespace Mirael
{

void Mirael::Settings::show(bool &open)
{
    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowName(), &open)) {
        if (ImGui::CollapsingHeader("Mirael Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            showMiraelSetingsEditor();
            ImGui::Separator();
        }

        if (ImGui::CollapsingHeader("Node Editor Style", ImGuiTreeNodeFlags_DefaultOpen)) {
            ax::NodeEditor::Utilities::ShowStyleEditor();
            ImGui::Separator();
        }

        if (ImGui::CollapsingHeader("ImGui Style", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ShowStyleEditor();
        }
    }
    ImGui::End();
}

void Settings::showMiraelSetingsEditor()
{
    App &app = App::get();

    ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;

    ImGui::SeparatorText("Change Tracking");

    ImGui::Text("Actions which mark a project unsaved:");
    ImGui::SameLine();
    ImGuiEx::ToolTipHint("The following actions always mark a project unsaved: "
                         "Adding/removing nodes and graphs, and adjusting node/graph settings.");

    auto &changeTrackingSettings = app.getChangeTrackingSettings();
    ImGui::Checkbox("Pan/Zoom of a Graph", &changeTrackingSettings.panZoom);
    ImGui::Checkbox("Moving a Node", &changeTrackingSettings.moveNode);
    ImGui::Checkbox("Toggling Graph Visiblity", &changeTrackingSettings.graphVisibility);
    
    ImGui::SeparatorText("Mirael Style Values");

    auto &values = app.getStyle().values;
    ImGui::SliderFloat("Pin Icon Size", &values.pinIconSize, 1.0f, 50.f, "%.1f");

    ImGui::SeparatorText("Mirael Style Colors");

    auto &colors = app.getStyle().colors;
    ImGui::ColorEdit4("Node Header", &colors.nodeHeaderFill.x);
    ImGui::ColorEdit4("Pin Icon", &colors.pinIconColor.x);
}

}; // namespace Mirael