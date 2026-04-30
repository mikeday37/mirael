#pragma once

#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

#include "data.h"
#include "graph.h"

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
    bool isModified() const { return isModifiedFlag; }

    static Project &get();

    void setLastFocusedGraphId(GraphId id) { lastFocusedGraphId = id; }
    void createNodeInLastFocusedGraphIfVisible(const char *nodeTypeName);

    // graph management
    Graph &addNewGraph();
    void removeGraph(GraphId id);
    Graph &getGraph(GraphId id) { return *graphMap.at(id); }
    const Graph &getGraph(GraphId id) const { return *graphMap.at(id); }
    std::span<GraphId> getGraphIdsInDisplayOrder();

    // serialization
    void save(const std::filesystem::path &filepath);
    [[nodiscard]] static std::unique_ptr<Project> load(const std::filesystem::path &filepath);
    std::optional<std::filesystem::path> getLastFilepath() const { return lastFilepath; }
    std::string getFileName() const {return fileName;}

private:
    // main data
    std::string name    = "Project";
    GraphId nextGraphId = 1;
    std::unordered_map<GraphId, std::unique_ptr<Graph>> graphMap;

    // interaction
    GraphId lastFocusedGraphId{};

    // modification tracking
    bool isModifiedFlag = false;
    void watchGraphChanges(Graph &graph);

    // serialization support
    std::optional<std::filesystem::path> lastFilepath;
    std::string fileName = "unnamed project";
    void storeFilepath(std::filesystem::path filepath);
    void serialize(nlohmann::json &j) const;
    [[nodiscard]] static std::unique_ptr<Project> deserialize(const nlohmann::json &j);

    // display ordering
    std::vector<GraphId> displayOrder;
    bool orderDirty = false;
    void updateDisplayOrder();
};

}; // namespace Mirael
