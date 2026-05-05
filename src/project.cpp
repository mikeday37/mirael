#include "pch.h"

#include "natural_sort.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "app.h"
#include "graph.h"
#include "project.h"

namespace Mirael
{

using json = nlohmann::json;

void Project::showGraphs()
{
    for (auto &[id, graph] : graphMap) {
        graph->showView();
    }
}

Project &Project::get() { return App::get().getProject(); }

void Project::createNodeInLastFocusedGraphIfVisible(const char *nodeTypeName)
{
    auto it = graphMap.find(lastFocusedGraphId);
    if (it != graphMap.end() && it->second->isVisible())
        it->second->userCreateNode(nodeTypeName);
}

Graph &Project::addNewGraph()
{
    auto uid            = App::get().getNewUuidAsString();
    auto id             = nextGraphId++;
    auto [it, inserted] = graphMap.emplace(id, std::make_unique<Graph>(id, uid));
    Graph &storedGraph  = *it->second;
    watchGraphChanges(storedGraph);
    isModifiedFlag = true;
    orderDirty     = true;
    storedGraph.rename(std::format("Graph {}", id));
    return storedGraph;
}

void Project::removeGraph(GraphId id)
{
    if (graphMap.erase(id)) {
        isModifiedFlag = true;
        orderDirty     = true;
    }
}

std::span<GraphId> Project::getGraphIdsInDisplayOrder()
{
    updateDisplayOrder();
    return displayOrder;
}

void Project::watchGraphChanges(Graph &graph)
{
    auto &changeTrackingSettings = App::get().getChangeTrackingSettings();
    graph.onModified             = [this, &changeTrackingSettings](ChangeImpact impact) {
        switch (impact) {

        case ChangeImpact::GraphName:
            isModifiedFlag = true;
            orderDirty     = true;
            break;

        case ChangeImpact::GraphVisibility:
            if (changeTrackingSettings.graphVisibility)
                isModifiedFlag = true;
            break;

        case ChangeImpact::NodePosition:
            if (changeTrackingSettings.moveNode)
                isModifiedFlag = true;
            break;

        case ChangeImpact::GraphPanZoom:
            if (changeTrackingSettings.panZoom)
                isModifiedFlag = true;
            break;

        case ChangeImpact::AddNode:
            [[fallthrough]];
        case ChangeImpact::RemoveNode:
            [[fallthrough]];
        case ChangeImpact::NodeConfig:
            isModifiedFlag = true;
            break;

        default:
            throw new std::runtime_error("Unhandled change impact.");
        }
    };
}

void Project::save(const std::filesystem::path &filepath)
{
    json j;
    serialize(j);
    std::ofstream o(filepath);
    if (!o.is_open())
        throw std::runtime_error("Failed to open file for writing: " + filepath.string());
    o << std::setw(4) << j;
    if (!o.good())
        throw std::runtime_error("Failed to write data to: " + filepath.string());
    isModifiedFlag = false;
    storeFilepath(filepath);
}

[[nodiscard]] std::unique_ptr<Project> Project::load(const std::filesystem::path &filepath)
{
    std::ifstream i(filepath);
    if (!i.is_open())
        throw std::runtime_error("Failed to open file for reading: " + filepath.string());
    json j;
    i >> j;
    // TODO: catch parse errors and fail gracefully with user notice
    auto project = deserialize(j);
    project->storeFilepath(filepath);
    return project;
}

void Project::storeFilepath(std::filesystem::path filepath)
{
    lastFilepath = filepath;
    fileName     = filepath.filename().string();
}

void Project::serialize(nlohmann::json &j) const
{
    assert(!orderDirty);
    j["graphs"] = json::object();
    for (const auto &[id, graph] : graphMap) {
        json graphJson;
        graph->serialize(graphJson);
        j["graphs"][std::to_string(id)] = graphJson;
    }
}

[[nodiscard]] std::unique_ptr<Project> Project::deserialize(const nlohmann::json &j)
{
    auto project          = std::make_unique<Project>();
    GraphId lastId        = 0;
    const auto &graphsObj = j.at("graphs");
    if (!graphsObj.is_object())
        throw std::runtime_error("Project json parsing error: 'graphs' is not an object.");

    for (const auto &[key, value] : graphsObj.items()) {
        GraphId id          = static_cast<GraphId>(std::stoull(key));
        auto uid            = value["uid"].get<std::string>();
        auto [it, inserted] = project->graphMap.try_emplace(id, Graph::deserialize(id, uid, value));
        if (!inserted)
            throw std::runtime_error(std::format("Graph Id {} not inserted during deserialization.", id));

        lastId = std::max(lastId, id);

        project->watchGraphChanges(*it->second);
    }

    project->nextGraphId    = lastId + 1;
    project->isModifiedFlag = false;
    project->orderDirty     = true;

    return project;
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
              [this](GraphId a, GraphId b) { return SI::natural::compare(graphMap.at(a)->getName(), graphMap.at(b)->getName()); });

    orderDirty = false;
}

}; // namespace Mirael