#pragma once

#include <filesystem>
#include <map>
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
    // forbid copy, allow move
    Project()                           = default;
    Project(const Project &)            = delete;
    Project &operator=(const Project &) = delete;
    Project(Project &&)                 = default;
    Project &operator=(Project &&)      = default;

    void showExplorer(bool &open);
    void showGraphs();
    bool isModified() const { return isModifiedFlag; }
    void resumeLastProject(std::filesystem::path filepath);
    std::optional<std::filesystem::path> getLastFilePath() const { return lastFilepath; }

private:
    // main data
    std::string name    = "Project";
    GraphId nextGraphId = 1;
    std::unordered_map<GraphId, Graph> graphMap;

    // graph management
    Graph &addGraph(Graph &&graph);
    Graph &addNewGraph();
    void removeGraph(GraphId id);
    Graph &getGraph(GraphId id) { return graphMap.at(id); }
    const Graph &getGraph(GraphId id) const { return graphMap.at(id); }

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
    ProjectData toData() const;
    static Project fromData(const ProjectData &data);
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
