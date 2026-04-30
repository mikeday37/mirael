#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "data.h"

namespace Mirael
{

class Project;

class ProjectExplorer
{
public:
    ProjectExplorer() : project(std::make_unique<Project>()) {}

    // forbid copy, move
    ProjectExplorer(const ProjectExplorer &)            = delete;
    ProjectExplorer &operator=(const ProjectExplorer &) = delete;
    ProjectExplorer(ProjectExplorer &&)                 = delete;
    ProjectExplorer &operator=(ProjectExplorer &&)      = delete;

    Project &getProject() { return *project; }

    static const char *explorerWindowName() { return "Project Explorer"; }
    void showExplorer();

    void load(std::filesystem::path filepath);

private:
    std::unique_ptr<Project> project;
    void clear();

    // ui entry points to serialization
    void onUserNew();
    void onUserOpen();
    void onUserSave();
    void onUserSaveAs();

    // serialization support
    void openViaFileDialog();
    void saveViaFileDialog();

    // graph renaming
    bool renaming           = false;
    bool firstRenameFrame   = false;
    GraphId renamingGraphId = ~0;
    std::string renameBuffer;
};

}; // namespace Mirael