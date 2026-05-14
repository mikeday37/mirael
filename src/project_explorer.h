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
    ProjectExplorer() : project_(std::make_unique<Project>()) {}

    // forbid copy, move
    ProjectExplorer(const ProjectExplorer &)            = delete;
    ProjectExplorer &operator=(const ProjectExplorer &) = delete;
    ProjectExplorer(ProjectExplorer &&)                 = delete;
    ProjectExplorer &operator=(ProjectExplorer &&)      = delete;

    Project &getProject() { return *project_; }

    static const char *windowName() { return "Project Explorer"; }
    void show();

    void load(std::filesystem::path filepath);

private:
    std::unique_ptr<Project> project_;
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
    std::optional<GraphId> renamingGraphId_ = ~0;
    bool firstRenameFrame_                  = false;
    std::string renameBuffer_;
};

}; // namespace Mirael