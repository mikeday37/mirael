#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

#include "data.h"
#include "node.h"

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
    explicit Graph(GraphId id) : id(id) {}

    // forbid copy, allow move
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

    void serialize(nlohmann::json &j) const;
    static Graph deserialize(GraphId id, const nlohmann::json &j);

    void setVisible(bool visible);
    bool isVisible() const { return visible; }
    std::string getWindowName() const { return windowName; }
    void bringWindowForward() const;
    void activate(); // sets visible and brings forward

    void showView();
    void raiseModified(ChangeImpact impact) const;

    GraphElementId getNextElementId() { return nextElementId++; }

    void userCreateNode(const char *typeName);

private:
    GraphId id;
    std::string name;
    bool visible = true;

    std::string windowName; // derived from id and name, but cached so it doesn't reallocate every frame
    void rebuildWindowName();
    std::string settingsFileName; // must be cached or the pointer in editor config would dangle

    struct EditorDeleter {
        void operator()(EditorContext *context) const;
    };
    std::unique_ptr<EditorContext, EditorDeleter> context;

    GraphElementId nextElementId = 1;
    std::unordered_map<NodeId, std::unique_ptr<Node>> nodes;
    std::unordered_map<LinkId, Link> links;
};

}; // namespace Mirael