#include "pch.h"

#include "misc/cpp/imgui_stdlib.h"

#include <filesystem>

#include "App.h"
#include "Graph.h"
#include "NfdShim.h"
#include "Project.h"
#include "ProjectExplorer.h"

namespace Mirael
{

namespace fs = std::filesystem;
using json   = nlohmann::json;

void ProjectExplorer::show()
{
    auto windowFlags = ImGuiWindowFlags_MenuBar | (project_->isModified() ? ImGuiWindowFlags_UnsavedDocument : 0);

    if (ImGui::Begin(windowName(), nullptr, windowFlags)) {

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    onUserNew();
                }
                if (ImGui::MenuItem("Open")) {
                    onUserOpen();
                }
                if (ImGui::MenuItem("Save")) {
                    onUserSave();
                }
                if (ImGui::MenuItem("Save As")) {
                    onUserSaveAs();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    App::get().exit(); // app handles confirmation
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Project")) {
                if (ImGui::MenuItem("Add New Graph")) {
                    project_->addNewGraph();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Node Library", nullptr, App::get().getLibraryFlag());
                ImGui::MenuItem("Properties", nullptr, App::get().getPropertiesFlag());
                ImGui::MenuItem("Settings", nullptr, App::get().getSettingsFlag());
                ImGui::MenuItem("Diagnostics", nullptr, App::get().getDiagnosticsFlag());
                ImGui::Separator();
                if (ImGui::MenuItem("Fullscreen", nullptr, App::get().getFullscreenFlag())) {
                    App::get().applyFullscreenSetting();
                }
                ImGui::Separator();
                ImGui::MenuItem("Show Dear ImGui Demo", nullptr, App::get().getImGuiDemoFlag());
                ImGui::MenuItem("Show ImPlot Demo", nullptr, App::get().getImPlotDemoFlag());
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("About", nullptr, nullptr, false);
                ImGui::Separator();
                if (ImGui::MenuItem("Reorient Selected Graph")) {
                    auto graph = project_->getLastFocusedGraph();
                    if (graph) {
                        graph->Reorient();
                    }
                }
                if (ImGui::MenuItem("Reposition Nodes in Selected Graph")) {
                    auto graph = project_->getLastFocusedGraph();
                    if (graph) {
                        graph->RepositionNodes();
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::TreeNodeEx(project_->getFileName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && project_->getLastFilepath()) {
                ImGui::SetTooltip(project_->getLastFilepath()->string().c_str());
            }
            const ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            for (auto id : project_->getGraphIdsInDisplayOrder()) {
                auto &graph = project_->getGraph(id);
                auto name   = graph.getName();
                if (renamingGraphId_ && *renamingGraphId_ == id) {
                    ImGui::TreeNodeEx("##renaming_item", leafFlags);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (firstRenameFrame_)
                        ImGui::SetKeyboardFocusHere();
                    if (ImGui::InputText("##rename", &renameBuffer_,
                                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                        graph.rename(renameBuffer_);
                        renamingGraphId_.reset();
                    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (!firstRenameFrame_ && !ImGui::IsItemActive())) {
                        renamingGraphId_.reset();
                    }
                    firstRenameFrame_ = false;
                } else {
                    ImGui::TreeNodeEx((void *)(intptr_t)id, leafFlags, "%.*s", name.size(), name.data());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        graph.activate();
                    }
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Show")) {
                            graph.activate();
                        }
                        if (ImGui::MenuItem("Hide")) {
                            graph.setVisible(false);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Rename")) {
                            renamingGraphId_  = id;
                            renameBuffer_     = name;
                            firstRenameFrame_ = true;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Delete")) {
                            App::get().setDestructiveAction(
                                "Delete Graph?", std::format("Delete graph \"{}\"?  This operation cannot be undone.", name),
                                [this, id]() { project_->removeGraph(id); });
                        }
                        ImGui::EndPopup();
                    }
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

bool ProjectExplorer::tryLoad(std::filesystem::path filepath)
{
    if (!filepath.empty() && fs::exists(filepath)) {
        project_ = Project::load(filepath);
        return true;
    } else
        return false;
}

void ProjectExplorer::attemptSetGraphFocus(GraphId graphId)
{
    if (project_ && project_->containsGraph(graphId))
        project_->getGraph(graphId).activate();
}

void ProjectExplorer::clear() { project_ = std::make_unique<Project>(); }

void ProjectExplorer::newProject()
{
    clear();
    project_->addNewGraph();
    project_->setNotModified();
}

void ProjectExplorer::onUserNew()
{
    if (project_->isModified()) {
        App::get().setDestructiveAction("Discard Changes?", "Discard ALL unsaved changes?  This operation cannot be undone.",
                                        [this]() { newProject(); });
    } else
        newProject();
}

void ProjectExplorer::onUserOpen()
{
    if (project_->isModified()) {
        App::get().setDestructiveAction("Discard Changes?", "Discard ALL unsaved changes?  This operation cannot be undone.",
                                        [this]() { openViaFileDialog(); });
    } else
        openViaFileDialog();
}

void ProjectExplorer::onUserSave()
{
    const auto &lastFilepath = project_->getLastFilepath();
    if (lastFilepath)
        project_->save(*lastFilepath);
    else
        saveViaFileDialog();
}

void ProjectExplorer::onUserSaveAs() { saveViaFileDialog(); }

void ProjectExplorer::openViaFileDialog()
{
    NfdShim::OpenArgs args = {.filters = {{"Mirael Projects", "mir"}}};
    auto results           = NfdShim::getOpenFilePath(args);
    if (results.good())
        project_ = Project::load(results.filepath);
    else if (results.bad()) {
        App::get().showError("Unable to choose path to open: " + results.errorMessage);
    }
}

void ProjectExplorer::saveViaFileDialog()
{
    NfdShim::SaveArgs args = {.filters = {{"Mirael Projects", "mir"}}, .defaultName = "project.mir"};
    auto results           = NfdShim::getSaveAsFilePath(args);
    if (results.good())
        project_->save(results.filepath);
    else if (results.bad()) {
        App::get().showError("Unable to choose file path to save as: " + results.errorMessage);
    }
}

} // namespace Mirael