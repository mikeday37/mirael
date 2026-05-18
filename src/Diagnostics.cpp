#include "pch.h"

#include "App.h"
#include "Diagnostics.h"
#include "ImGuiEx.h"

namespace Mirael
{

void Mirael::Diagnostics::show(bool &open)
{
    const auto &io = ImGui::GetIO();

    ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;

    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowName(), &open)) {
        if (ImGui::CollapsingHeader("Mirael Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("##diags", 2, tableFlags)) {
                App::get().showDiagnosticRows();
                ImGui::EndTable();
            }
        }
        if (ImGui::CollapsingHeader("Last Selected Graph", ImGuiTreeNodeFlags_DefaultOpen)) {
            Graph *graph = Project::get().getLastFocusedGraph();
            if (graph) {
                if (ImGui::BeginTable("##diags", 2, tableFlags)) {
                    graph->showDiagnosticRows();
                    ImGui::EndTable();
                }
            } else {
                ImGui::TextUnformatted("n/a");
            }
        }
        if (ImGui::CollapsingHeader("ImGui IO", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("##diags", 2, tableFlags)) {

                ImGuiEx::RowLabel("Mouse X");
                ImGui::Text("%.0f", io.MousePos.x);

                ImGuiEx::RowLabel("Mouse Y");
                ImGui::Text("%.0f", io.MousePos.y);

                ImGui::EndTable();
            }
        }
        if (ImGui::CollapsingHeader("ImGui Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("##diags", 2, tableFlags)) {

                ImGuiEx::RowLabel("Framerate", "Average of last 60 frames.");
                ImGui::Text("%.1f", io.Framerate);

                ImGuiEx::RowLabel("Active Windows");
                ImGui::Text("%d", io.MetricsActiveWindows);

                ImGuiEx::RowLabel("Render Windows");
                ImGui::Text("%d", io.MetricsRenderWindows);

                ImGuiEx::RowLabel("Render Vertices");
                ImGui::Text("%d", io.MetricsRenderVertices);

                ImGuiEx::RowLabel("Render Indices");
                ImGui::Text("%d", io.MetricsRenderIndices);

                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

} // namespace Mirael