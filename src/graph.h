#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "data.h"

namespace ax::NodeEditor
{
struct EditorContext;
};

namespace Mirael
{

using EditorContext = ::ax::NodeEditor::EditorContext;

class Graph
{
public:
    // forbid copy, allow move
    explicit Graph(GraphId id) : id(id) {}
    Graph(const Graph &)            = delete;
    Graph &operator=(const Graph &) = delete;
    Graph(Graph &&)                 = default;
    Graph &operator=(Graph &&)      = default;

    enum class ChangeImpact {
        Name, // used if the name changed (may include other changes)
        Other // used only if the name did not change
    };
    using ChangeCallback = std::function<void(ChangeImpact)>;
    ChangeCallback onModified;

    void rename(std::string newName);
    std::string_view getName() const { return name; }

    GraphData toData() const;
    static Graph fromData(GraphId id, const GraphData &data);

    void setVisible(bool visible);
    std::string getWindowName() const;
    void bringWindowForward() const;
    void activate(); // sets visible and brings forward

    void showView(GraphId id);
    void raiseModified(ChangeImpact impact) const;

private:
    GraphId id;
    std::string name = "Graph";
    bool visible     = true;

    struct EditorDeleter {
        void operator()(EditorContext *context) const;
    };
    std::unique_ptr<EditorContext, EditorDeleter> context;
};

}; // namespace Mirael