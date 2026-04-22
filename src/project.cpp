#include "pch.h"

#include "misc/cpp/imgui_stdlib.h"
#include "natural_sort.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "app.h"
#include "graph.h"
#include "nfd_shim.h"
#include "project.h"

namespace Mirael
{

namespace fs = std::filesystem;
using json   = nlohmann::json;

void Project::showExplorer(bool &open)
{
    auto windowFlags = ImGuiWindowFlags_MenuBar | (isModifiedFlag ? ImGuiWindowFlags_UnsavedDocument : 0);

    if (ImGui::Begin("Project Explorer", &open, windowFlags)) {

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
                    addNewGraph();
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
        updateDisplayOrder();
        for (auto id : displayOrder) {
            auto &graph = getGraph(id);
            auto name   = graph.getName();
            if (renaming && renamingGraphId == id) {
                ImGui::TreeNodeEx("##renaming_item", leafFlags);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (firstRenameFrame)
                    ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##rename", &renameBuffer,
                                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                    graph.rename(renameBuffer);
                    renaming = false;
                } else if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (!firstRenameFrame && !ImGui::IsItemActive())) {
                    renaming = false;
                }
                firstRenameFrame = false;
            } else {
                ImGui::TreeNodeEx((void *)(intptr_t)id, leafFlags, "%.*s", name.size(), name.data());
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Rename")) {
                        renaming         = true;
                        renamingGraphId  = id;
                        renameBuffer     = name;
                        firstRenameFrame = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete")) {
                        App::get().setDestructiveAction("Delete Graph?",
                                                        std::format("Delete graph \"{}\"?  This operation cannot be undone.", name),
                                                        [this, id]() { removeGraph(id); });
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }
    ImGui::End();
}

void Project::resumeLastProject(std::filesystem::path filepath)
{
    if (!filepath.empty() && std::filesystem::exists(filepath))
        load(filepath);
}

Graph &Project::addGraph(Graph &&graph)
{
    auto id             = nextGraphId++;
    auto [it, inserted] = graphMap.emplace(id, std::move(graph));
    Graph &storedGraph  = it->second;
    watchGraphChanges(storedGraph);
    isModifiedFlag = true;
    orderDirty     = true;
    return storedGraph;
}

Graph &Project::addNewGraph()
{
    auto id      = nextGraphId;
    Graph &graph = addGraph(Graph{});
    graph.rename(std::format("Graph {}", id));
    return graph;
}

void Project::removeGraph(GraphId id)
{
    if (graphMap.erase(id)) {
        isModifiedFlag = true;
        orderDirty     = true;
    }
}

void Project::watchGraphChanges(Graph &graph)
{
    graph.onModified = [this](Graph::ChangeImpact impact) {
        isModifiedFlag = true;
        if (impact == Graph::ChangeImpact::Name)
            orderDirty = true;
    };
}

void Project::onUserNew()
{
    if (isModifiedFlag) {
        App::get().setDestructiveAction("Discard Changes?", "Discard ALL unsaved changes?  This operation cannot be undone.",
                                        [this]() { clear(); });
    } else
        clear();
}

void Project::onUserOpen()
{
    if (isModifiedFlag) {
        App::get().setDestructiveAction("Discard Changes?", "Discard ALL unsaved changes?  This operation cannot be undone.",
                                        [this]() { openViaFileDialog(); });
    } else
        openViaFileDialog();
}

void Project::onUserSave()
{
    if (lastFilepath)
        save(*lastFilepath);
    else
        saveViaFileDialog();
}

void Project::onUserSaveAs() { saveViaFileDialog(); }

void Project::openViaFileDialog()
{
    NfdShim::OpenArgs args = {.filters = {{"Mirael Projects", "mir"}}};
    auto results = NfdShim::getOpenFilePath(args);
    if (results.good())
        load(results.filepath);
    else if (results.bad()) {
        App::get().showError("Unable to choose path to open: " + results.errorMessage);
    }
}

void Project::saveViaFileDialog()
{
    NfdShim::SaveArgs args = {.filters = {{"Mirael Projects", "mir"}}, .defaultName = "project.mir"};
    auto results = NfdShim::getSaveAsFilePath(args);
    if (results.good())
        save(results.filepath);
    else if (results.bad()) {
        App::get().showError("Unable to choose file path to save as: " + results.errorMessage);
    }
}

void Project::clear()
{
    name        = "Project";
    nextGraphId = 1;
    graphMap.clear();
    isModifiedFlag = false;

    lastFilepath.reset();

    displayOrder.clear();
    orderDirty = false;

    renaming         = false;
    firstRenameFrame = false;
    renamingGraphId  = ~0;
    renameBuffer.clear();
}

void Project::save(const std::filesystem::path &filepath)
{
    const auto data = toData();
    std::ofstream o(filepath);
    if (!o.is_open())
        throw std::runtime_error("Failed to open file for writing: " + filepath.string());
    o << std::setw(4) << json(data);
    if (!o.good())
        throw std::runtime_error("Failed to write data to: " + filepath.string());
    isModifiedFlag = false;
    lastFilepath = filepath;
}

void Project::load(const std::filesystem::path &filepath)
{
    std::ifstream i(filepath);
    if (!i.is_open())
        throw std::runtime_error("Failed to open file for reading: " + filepath.string());
    json j;
    i >> j;
    auto data = j.get<ProjectData>();
    // TODO: catch parse errors and fail gracefully with user notice
    *this = fromData(data);
    connectCallbacks();
    lastFilepath = filepath;
}

ProjectData Project::toData() const
{
    assert(!orderDirty);
    ProjectData data;
    for (const auto &[id, graph] : graphMap) {
        data.graphs.try_emplace(id, graph.toData());
    }
    return data;
}

Project Project::fromData(const ProjectData &data)
{
    Project project;
    GraphId lastId = 0;
    for (const auto &[id, graphData] : data.graphs) {
        auto [it, inserted] = project.graphMap.try_emplace(id, Graph::fromData(graphData));
        if (!inserted)
            throw std::runtime_error(std::format("Graph Id {} not inserted during deserialization.", id));
        if (id > lastId)
            lastId = id;
    }
    project.nextGraphId    = lastId + 1;
    project.isModifiedFlag = false;
    project.orderDirty     = true;
    return project;
}

void Project::connectCallbacks()
{
    for (auto &[id, graph] : graphMap)
        watchGraphChanges(graph);
}

void Project::updateDisplayOrder()
{
    if (!orderDirty)
        return;

    displayOrder.clear();
    displayOrder.reserve(graphMap.size());

    for (const auto &[id, _] : graphMap) {
        displayOrder.push_back(id);
    }

    std::sort(displayOrder.begin(), displayOrder.end(),
              [this](GraphId a, GraphId b) { return SI::natural::compare(graphMap.at(a).getName(), graphMap.at(b).getName()); });

    orderDirty = false;
}

}; // namespace Mirael