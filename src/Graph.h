#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "data.h"
#include "Node.h"
#include "Runner.h"

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
    explicit Graph(GraphId id, std::string_view uid) : id_(id), uid_(uid) {}

    // forbid copy, move
    Graph(const Graph &)            = delete;
    Graph &operator=(const Graph &) = delete;
    Graph(Graph &&)                 = delete;
    Graph &operator=(Graph &&)      = delete;

    using ChangeCallback = std::function<void(ChangeImpact)>;
    ChangeCallback onModified;

    void rename(std::string newName);
    std::string_view getName() const { return name_; }

    void serialize(nlohmann::json &j) const;
    static std::unique_ptr<Graph> deserialize(GraphId id, std::string_view uid, const nlohmann::json &j);
    void serializeLink(nlohmann::json &j, const Link &link) const;
    Link deserializeLink(const nlohmann::json &j);

    void setVisible(bool visible);
    bool isVisible() const { return visible_; }
    std::string getWindowName() const { return windowName_; }
    void bringWindowForward() const;
    void activate(); // sets visible and brings forward

    void showView();
    void raiseModified(ChangeImpact impact) const;

    GraphElementId getNextElementId() { return nextElementId_++; }

    void userCreateNode(const char *typeName);

    void showDiagnosticRows();

    ImVec2 getCanvasViewCenter() const;

    // temporary debug helpers - may be removed
    void RepositionNodes();
    void Reorient();

    void showProperties();

    enum class SelectionStatus { None, SingleLink, SingleNode, Multiple };
    static const char *to_string(SelectionStatus status);
    SelectionStatus getSelectionStatus() const { return selectionStatus_; }
    Node *getSingleSelectedNode();

    // to be called only by base Node:
    void onPinAdded(NodeId nodeId, PinId pinId, PinConfig pinConfig);
    void onPinRemoved(NodeId nodeId, PinId pinId);

    static const char *to_display_string(RunRateMode mode);
    static const char *to_string(RunRateMode mode);
    static bool try_parse(std::string_view s, RunRateMode &mode);

private:
    GraphId id_;
    std::string uid_;
    std::string name_;
    bool visible_           = true;
    RunRateMode runRateMode_ = RunRateMode::SetRate;
    float frameRateSetting_ = 60.0f;

    std::string windowName_; // derived from id and name, but cached so it doesn't reallocate every frame
    void rebuildWindowName();

    std::string editorId_;
    struct EditorDeleter {
        void operator()(EditorContext *context) const;
    };
    std::unique_ptr<EditorContext, EditorDeleter> context_;
    void initEditorContext();
    void adjustEditorStyle();

    GraphElementId nextElementId_ = 1;
    struct PinInfo {
        NodeId nodeId;
        PinDirection direction;
    };
    std::unordered_map<NodeId, std::unique_ptr<Node>> nodes_;
    std::unordered_map<LinkId, Link> links_;
    std::unordered_map<PinId, PinInfo> pins_;
    std::unordered_map<PinId, std::unordered_set<LinkId>> pinLinks_;
    PinInfo getPinInfo(PinId pinId) const { return pins_.at(pinId); }
    void addLink(Link &&link);
    void addLinkWithId(Link &&link, LinkId linkId);
    void removeLink(LinkId linkId);

    // editor wrangling
    struct CanvasOrientation {
        float zoom = 1.0f;
        ImVec2 origin;
    };
    std::optional<CanvasOrientation> pendingSetCanvasOrientation_{}, pendingSetInitialCanvasOrientation_{};

    struct CanvasInfo {
        CanvasOrientation orientation;
        ImVec2 mousePos;
        ImVec2 viewRectMin, viewRectMax;
    };
    CanvasInfo canvasInfo_{};

    static bool isOrientationChangeSignificant(CanvasOrientation a, CanvasOrientation b);

    SelectionStatus selectionStatus_ = SelectionStatus::None;
    std::optional<NodeId> selectedNodeId_;
    std::optional<LinkId> selectedLinkId_;
    void processSelectionState(); // called within ne::Begin()/::End()

    void showNodesAndLinks();

    void removeNode(NodeId nodeId);
};

}; // namespace Mirael