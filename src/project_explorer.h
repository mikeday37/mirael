#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

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

    static const char *windowName() { return "Project Explorer"; }
    void show();

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
    std::optional<GraphId> renamingGraphId = ~0;
    bool firstRenameFrame   = false;
    std::string renameBuffer;
};

}; // namespace Mirael