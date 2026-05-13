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
    explicit Graph(GraphId id, std::string_view uid) : id(id), uid(uid) {}

    // forbid copy, move
    Graph(const Graph &)            = delete;
    Graph &operator=(const Graph &) = delete;
    Graph(Graph &&)                 = delete;
    Graph &operator=(Graph &&)      = delete;

    using ChangeCallback = std::function<void(ChangeImpact)>;
    ChangeCallback onModified;

    void rename(std::string newName);
    std::string_view getName() const { return name; }

    void serialize(nlohmann::json &j) const;
    static std::unique_ptr<Graph> deserialize(GraphId id, std::string_view uid, const nlohmann::json &j);

    void setVisible(bool visible);
    bool isVisible() const { return visible; }
    std::string getWindowName() const { return windowName; }
    void bringWindowForward() const;
    void activate(); // sets visible and brings forward

    void showView();
    void raiseModified(ChangeImpact impact) const;

    GraphElementId getNextElementId() { return nextElementId++; }

    void userCreateNode(const char *typeName);

    void showDiagnosticRows();

    ImVec2 getCanvasViewCenter() const;

    // temporary debug helpers - may be removed
    void RepositionNodes();
    void Reorient();

    void showProperties();

    enum SelectionStatus { None, SingleLink, SingleNode, Multiple };
    static const char *to_string(SelectionStatus status);
    SelectionStatus getSelectionStatus() const { return selectionStatus; }
    Node *getSingleSelectedNode();

    // to be called only by base Node:
    void onPinAdded(NodeId nodeId, PinId pinId, PinConfig pinConfig);
    void onPinRemoved(NodeId nodeId, PinId pinId);

private:
    GraphId id;
    std::string uid;
    std::string name;
    bool visible = true;

    std::string windowName; // derived from id and name, but cached so it doesn't reallocate every frame
    void rebuildWindowName();

    std::string editorId;
    struct EditorDeleter {
        void operator()(EditorContext *context) const;
    };
    std::unique_ptr<EditorContext, EditorDeleter> context;
    void initEditorContext();
    void adjustEditorStyle();

    GraphElementId nextElementId = 1;
    struct PinInfo {
        NodeId nodeId;
        PinDirection direction;
    };
    std::unordered_map<NodeId, std::unique_ptr<Node>> nodes;
    std::unordered_map<LinkId, Link> links;
    std::unordered_map<PinId, PinInfo> pins;
    std::unordered_map<PinId, std::unordered_set<LinkId>> pinLinks;
    PinInfo getPinInfo(PinId pinId) const { return pins.at(pinId); }
    void addLink(Link &&link);
    void removeLink(LinkId linkId);

    // editor wrangling
    struct CanvasOrientation {
        float zoom = 1.0f;
        ImVec2 origin;
    };
    std::optional<CanvasOrientation> pendingSetCanvasOrientation{}, pendingSetInitialCanvasOrientation{};

    struct CanvasInfo {
        CanvasOrientation orientation;
        ImVec2 mousePos;
        ImVec2 viewRectMin, viewRectMax;
    };
    CanvasInfo canvasInfo{};

    static bool isOrientationChangeSignificant(CanvasOrientation a, CanvasOrientation b);

    SelectionStatus selectionStatus = SelectionStatus::None;
    std::optional<NodeId> selectedNodeId;
    std::optional<LinkId> selectedLinkId;
    void processSelectionState(); // called within ne::Begin()/::End()
};

}; // namespace Mirael