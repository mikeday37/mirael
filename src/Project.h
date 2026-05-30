#pragma once

#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

#include "data.h"
#include "Graph.h"

namespace Mirael
{

class Project
{
public:
    Project() = default;

    // forbid copy, move
    Project(const Project &)            = delete;
    Project &operator=(const Project &) = delete;
    Project(Project &&)                 = delete;
    Project &operator=(Project &&)      = delete;

    void showGraphs();
    bool isModified() const { return isModifiedFlag_; }
    void setNotModified() { isModifiedFlag_ = false; }

    static Project &get();

    void setLastFocusedGraphId(GraphId id) { lastFocusedGraphId_ = id; }
    Graph *getLastFocusedGraph() { return graphMap_.contains(lastFocusedGraphId_) ? &getGraph(lastFocusedGraphId_) : nullptr; }
    std::optional<GraphId> getLastFocusedGraphId() const;
    void createNodeInLastFocusedGraphIfVisible(const char *nodeTypeName);

    // graph management
    Graph &addNewGraph();
    void removeGraph(GraphId id);
    Graph &getGraph(GraphId id) { return *graphMap_.at(id); }
    const Graph &getGraph(GraphId id) const { return *graphMap_.at(id); }
    std::span<GraphId> getGraphIdsInDisplayOrder();
    bool containsGraph(GraphId id) const { return graphMap_.contains(id); }

    // serialization
    void save(const std::filesystem::path &filepath);
    [[nodiscard]] static std::unique_ptr<Project> load(const std::filesystem::path &filepath);
    std::optional<std::filesystem::path> getLastFilepath() const { return lastFilepath_; }
    std::string getFileName() const { return fileName_; }

private:
    // main data
    GraphId nextGraphId_ = 1;
    std::unordered_map<GraphId, std::unique_ptr<Graph>> graphMap_;

    // interaction
    GraphId lastFocusedGraphId_{};

    // modification tracking
    bool isModifiedFlag_ = false;
    void watchGraphChanges(Graph &graph);

    // serialization support
    std::optional<std::filesystem::path> lastFilepath_;
    std::string fileName_ = "unnamed project";
    void storeFilepath(std::filesystem::path filepath);
    void serialize(nlohmann::json &j) const;
    [[nodiscard]] static std::unique_ptr<Project> deserialize(const nlohmann::json &j);

    // display ordering
    std::vector<GraphId> displayOrder_;
    bool orderDirty_ = false;
    void updateDisplayOrder();
};

} // namespace Mirael
