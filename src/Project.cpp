#include "pch.h"

#include "natural_sort.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "App.h"
#include "Graph.h"
#include "Project.h"

namespace Mirael
{

using json = nlohmann::json;

void Project::showGraphs()
{
    for (auto &[id, graph] : graphMap_) {
        graph->showView();
    }
}

Project &Project::get() { return App::get().getProject(); }

std::optional<GraphId> Project::getLastFocusedGraphId() const
{
    if (graphMap_.contains(lastFocusedGraphId_))
        return lastFocusedGraphId_;
    else
        return std::nullopt;
}

void Project::createNodeInLastFocusedGraphIfVisible(const char *nodeTypeName)
{
    auto it = graphMap_.find(lastFocusedGraphId_);
    if (it != graphMap_.end() && it->second->isVisible())
        it->second->userCreateNode(nodeTypeName);
}

Graph &Project::addNewGraph()
{
    auto uid            = App::get().getNewUuidAsString();
    auto id             = nextGraphId_++;
    auto [it, inserted] = graphMap_.try_emplace(id, std::make_unique<Graph>(id, uid));
    Graph &storedGraph  = *it->second;
    watchGraphChanges(storedGraph);
    isModifiedFlag_ = true;
    orderDirty_     = true;
    storedGraph.rename(std::format("Graph {}", id));
    storedGraph.initRunner();
    return storedGraph;
}

void Project::removeGraph(GraphId id)
{
    if (graphMap_.erase(id)) {
        isModifiedFlag_ = true;
        orderDirty_     = true;
    }
}

std::span<GraphId> Project::getGraphIdsInDisplayOrder()
{
    updateDisplayOrder();
    return displayOrder_;
}

void Project::watchGraphChanges(Graph &graph)
{
    auto &changeTrackingSettings = App::get().getChangeTrackingSettings();
    graph.onModified             = [this, &changeTrackingSettings](ChangeImpact impact) {
        switch (impact) {

        case ChangeImpact::GraphName:
            isModifiedFlag_ = true;
            orderDirty_     = true;
            break;

        case ChangeImpact::GraphVisibility:
            if (changeTrackingSettings.graphVisibility)
                isModifiedFlag_ = true;
            break;

        case ChangeImpact::NodePosition:
            if (changeTrackingSettings.moveNode)
                isModifiedFlag_ = true;
            break;

        case ChangeImpact::GraphPanZoom:
            if (changeTrackingSettings.panZoom)
                isModifiedFlag_ = true;
            break;

        case ChangeImpact::AddNode:
            [[fallthrough]];
        case ChangeImpact::RemoveNode:
            [[fallthrough]];
        case ChangeImpact::AddLink:
            [[fallthrough]];
        case ChangeImpact::RemoveLink:
            [[fallthrough]];
        case ChangeImpact::NodeConfig:
            [[fallthrough]];
        case ChangeImpact::GraphRunRate:
            isModifiedFlag_ = true;
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
    isModifiedFlag_ = false;
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

    for (auto &[id, graph] : project->graphMap_)
        graph->initRunner();

    return project;
}

void Project::shutdown()
{
    std::vector<GraphId> ids;
    for (auto &[id, graph] : graphMap_) {
        graph->stopRunner();
        ids.push_back(id);
    }
    for (auto id : ids)
        removeGraph(id);
}

void Project::storeFilepath(std::filesystem::path filepath)
{
    lastFilepath_ = filepath;
    fileName_     = filepath.filename().string();
}

void Project::serialize(nlohmann::json &j) const
{
    j["graphs"] = json::object();
    for (const auto &[id, graph] : graphMap_) {
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
        auto [it, inserted] = project->graphMap_.try_emplace(id, Graph::deserialize(id, uid, value));
        if (!inserted)
            throw std::runtime_error(std::format("Graph Id {} not inserted during deserialization.", id));

        lastId = std::max(lastId, id);

        project->watchGraphChanges(*it->second);
    }

    project->nextGraphId_    = lastId + 1;
    project->isModifiedFlag_ = false;
    project->orderDirty_     = true;

    return project;
}

void Project::updateDisplayOrder()
{
    if (!orderDirty_)
        return;

    displayOrder_.clear();
    displayOrder_.reserve(graphMap_.size());

    for (const auto &[id, _] : graphMap_) {
        displayOrder_.push_back(id);
    }

    std::sort(displayOrder_.begin(), displayOrder_.end(),
              [this](GraphId a, GraphId b) { return SI::natural::compare(graphMap_.at(a)->getName(), graphMap_.at(b)->getName()); });

    orderDirty_ = false;
}

} // namespace Mirael