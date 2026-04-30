#include "pch.h"

#include "misc/cpp/imgui_stdlib.h"

#include <filesystem>

#include "app.h"
#include "graph.h"
#include "nfd_shim.h"
#include "project.h"
#include "project_explorer.h"

namespace Mirael
{

namespace fs = std::filesystem;
using json   = nlohmann::json;

void ProjectExplorer::showExplorer()
{
    auto windowFlags = ImGuiWindowFlags_MenuBar | (project->isModified() ? ImGuiWindowFlags_UnsavedDocument : 0);

    if (ImGui::Begin(explorerWindowName(), nullptr, windowFlags)) {

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
                    project->addNewGraph();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Show Dear ImGui Demo", nullptr, App::get().getImGuiDemoFlag());
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("About", nullptr, nullptr, false);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        const ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        for (auto id : project->getGraphIdsInDisplayOrder()) {
            auto &graph = project->getGraph(id);
            auto name   = graph.getName();
            if (renamingGraphId && *renamingGraphId == id) {
                ImGui::TreeNodeEx("##renaming_item", leafFlags);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (firstRenameFrame)
                    ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##rename", &renameBuffer,
                                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                    graph.rename(renameBuffer);
                    renamingGraphId.reset();
                } else if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (!firstRenameFrame && !ImGui::IsItemActive())) {
                    renamingGraphId.reset();
                }
                firstRenameFrame = false;
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
                        renamingGraphId  = id;
                        renameBuffer     = name;
                        firstRenameFrame = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete")) {
                        App::get().setDestructiveAction("Delete Graph?",
                                                        std::format("Delete graph \"{}\"?  This operation cannot be undone.", name),
                                                        [this, id]() { project->removeGraph(id); });
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }
    ImGui::End();
}

void ProjectExplorer::load(std::filesystem::path filepath)
{
    if (!filepath.empty() && fs::exists(filepath))
        project = Project::load(filepath);
}

void ProjectExplorer::clear() { project = std::make_unique<Project>(); }

void ProjectExplorer::onUserNew()
{
    if (project->isModified()) {
        App::get().setDestructiveAction("Discard Changes?", "Discard ALL unsaved changes?  This operation cannot be undone.",
                                        [this]() { clear(); });
    } else
        clear();
}

void ProjectExplorer::onUserOpen()
{
    if (project->isModified()) {
        App::get().setDestructiveAction("Discard Changes?", "Discard ALL unsaved changes?  This operation cannot be undone.",
                                        [this]() { openViaFileDialog(); });
    } else
        openViaFileDialog();
}

void ProjectExplorer::onUserSave()
{
    const auto &lastFilepath = project->getLastFilepath();
    if (lastFilepath)
        project->save(*lastFilepath);
    else
        saveViaFileDialog();
}

void ProjectExplorer::onUserSaveAs() { saveViaFileDialog(); }

void ProjectExplorer::openViaFileDialog()
{
    NfdShim::OpenArgs args = {.filters = {{"Mirael Projects", "mir"}}};
    auto results           = NfdShim::getOpenFilePath(args);
    if (results.good())
        project = Project::load(results.filepath);
    else if (results.bad()) {
        App::get().showError("Unable to choose path to open: " + results.errorMessage);
    }
}

void ProjectExplorer::saveViaFileDialog()
{
    NfdShim::SaveArgs args = {.filters = {{"Mirael Projects", "mir"}}, .defaultName = "project.mir"};
    auto results           = NfdShim::getSaveAsFilePath(args);
    if (results.good())
        project->save(results.filepath);
    else if (results.bad()) {
        App::get().showError("Unable to choose file path to save as: " + results.errorMessage);
    }
}

}; // namespace Mirael