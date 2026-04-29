#pragma once

#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
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

    // forbid copy, allow move
    Project(const Project &)            = delete;
    Project &operator=(const Project &) = delete;
    Project(Project &&)                 = default;
    Project &operator=(Project &&)      = default;

    static const char *explorerWindowName() { return "Project Explorer"; }
    void showExplorer();

    void showGraphs();
    bool isModified() const { return isModifiedFlag; }
    void resumeLastProject(std::filesystem::path filepath);
    std::optional<std::filesystem::path> getLastFilePath() const { return lastFilepath; }

    static Project &get();

    void setLastFocusedGraphId(GraphId id) { lastFocusedGraphId = id; }
    void createNodeInLastFocusedGraphIfVisible(const char *nodeTypeName);

private:
    // main data
    std::string name    = "Project";
    GraphId nextGraphId = 1;
    std::unordered_map<GraphId, std::unique_ptr<Graph>> graphMap;

    // graph management
    Graph &addGraph(std::unique_ptr<Graph> &&graph);
    Graph &addNewGraph();
    void removeGraph(GraphId id);
    Graph &getGraph(GraphId id) { return *graphMap.at(id); }
    const Graph &getGraph(GraphId id) const { return *graphMap.at(id); }

    // interaction
    GraphId lastFocusedGraphId{};

    // modification tracking
    bool isModifiedFlag = false;
    void watchGraphChanges(Graph &graph);

    // ui entry points to serialization
    void onUserNew();
    void onUserOpen();
    void onUserSave();
    void onUserSaveAs();

    // serialization support
    void openViaFileDialog();
    void saveViaFileDialog();
    void clear();
    void save(const std::filesystem::path &filepath);
    void load(const std::filesystem::path &filepath);
    std::optional<std::filesystem::path> lastFilepath;
    void serialize(nlohmann::json &j) const;
    static Project deserialize(const nlohmann::json &j);
    void connectCallbacks();

    // display ordering
    std::vector<GraphId> displayOrder;
    bool orderDirty = false;
    void updateDisplayOrder();

    // graph renaming
    bool renaming           = false;
    bool firstRenameFrame   = false;
    GraphId renamingGraphId = ~0;
    std::string renameBuffer;
};

}; // namespace Mirael
