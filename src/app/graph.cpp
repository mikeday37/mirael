#include "app_pch.hpp"

#include "app/graph.hpp"
#include "vec2.hpp"
#include <limits>
#include <map>
#include <vector>

inline std::pair<int, int> CanonicalEdge(int a, int b) { return a < b ? std::make_pair(a, b) : std::make_pair(b, a); }

int Graph::AddNode(glm::vec2 pos)
{
    assert(nextNodeId_ < std::numeric_limits<int>::max());

    int nodeId = nextNodeId_++;
    nodes_[nodeId] = pos;
    nodeEdges_[nodeId];

    return nodeId;
}

bool Graph::ContainsNode(int nodeId) const { return nodes_.contains(nodeId); }

Graph::Node Graph::GetNode(int nodeId) const
{
    assert(nodes_.contains(nodeId));

    auto it = nodes_.find(nodeId);

    return {nodeId, it->second};
}

std::vector<Graph::Node> Graph::GetNodes() const
{
    std::vector<Graph::Node> nodes;
    nodes.reserve(nodes_.size());

    for (const auto &[id, pos] : nodes_) {
        nodes.emplace_back(id, pos);
    }

    return nodes;
}

void Graph::RepositionNode(int nodeId, glm::vec2 pos)
{
    assert(nodes_.contains(nodeId));

    auto it = nodes_.find(nodeId);

    it->second.x = pos.x;
    it->second.y = pos.y;
}

void Graph::RemoveNode(int nodeId)
{
    assert(nodes_.contains(nodeId));
    assert(nodeEdges_.contains(nodeId));

    // get the edges connected to this node
    const auto &nodeEdges = nodeEdges_[nodeId];

    // cache it before removing edges
    std::vector<int> connectedEdges;
    connectedEdges.reserve(nodeEdges.size());
    for (const auto &edgeId : nodeEdges) {
        connectedEdges.emplace_back(edgeId);
    }

    // for each edge, erase it from other nodes' edge sets, and erase the edge itself
    for (const auto &edgeId : connectedEdges) {
        const auto &[nodeIdA, nodeIdB] = edges_[edgeId];
        int otherNodeId = nodeIdA == nodeId ? nodeIdB : nodeIdA;

        assert(edges_.contains(edgeId));
        assert(nodeEdges_[otherNodeId].contains(edgeId));

        nodeEdges_[otherNodeId].erase(edgeId);
        edgeSet_.erase(CanonicalEdge(nodeIdA, nodeIdB));
        edges_.erase(edgeId);
    }

    // finally erase the edge set itself for this node, and erase the node.
    nodeEdges_.erase(nodeId);
    nodes_.erase(nodeId);
}

int Graph::AddEdge(int nodeIdA, int nodeIdB)
{
    assert(nodeIdA != nodeIdB);
    assert(nodes_.contains(nodeIdA));
    assert(nodes_.contains(nodeIdB));

    if (ContainsEdge(nodeIdA, nodeIdB)) {
        return 0;
    }

    int edgeId = nextEdgeId_++;

    auto edgeKey = CanonicalEdge(nodeIdA, nodeIdB);
    edges_[edgeId] = edgeKey;
    nodeEdges_[nodeIdA].emplace(edgeId);
    nodeEdges_[nodeIdB].emplace(edgeId);
    edgeSet_.emplace(edgeKey);

    return edgeId;
}

bool Graph::ContainsEdge(int edgeId) const { return edges_.contains(edgeId); }

bool Graph::ContainsEdge(int nodeIdA, int nodeIdB) const
{
    assert(nodeIdA != nodeIdB);

    return edgeSet_.contains(CanonicalEdge(nodeIdA, nodeIdB));
}

Graph::Edge Graph::GetEdge(int edgeId) const
{
    assert(edges_.contains(edgeId));

    auto it = edges_.find(edgeId);

    return {edgeId, it->second.first, it->second.second};
}

std::vector<Graph::Edge> Graph::GetEdges() const
{
    std::vector<Graph::Edge> edges;
    edges.reserve(edges_.size());

    for (const auto &[id, nodes] : edges_) {
        edges.emplace_back(id, nodes.first, nodes.second);
    }

    return edges;
}

std::vector<Graph::Edge> Graph::GetEdges(int nodeId) const
{
    assert(nodes_.contains(nodeId));

    std::vector<Graph::Edge> edges;

    auto edgesIt = nodeEdges_.find(nodeId);
    if (edgesIt == nodeEdges_.end()) {
        return edges;
    }

    auto nodeEdgeIds = edgesIt->second;

    edges.reserve(nodeEdgeIds.size());

    for (const auto edgeId : nodeEdgeIds) {
        auto &edge = *edges_.find(edgeId);
        edges.emplace_back(edgeId, edge.second.first, edge.second.second);
    }

    return edges;
}

void Graph::RemoveEdge(int edgeId)
{
    assert(edges_.contains(edgeId));

    const auto &nodePair = edges_[edgeId];

    assert(nodes_.contains(nodePair.first));
    assert(nodes_.contains(nodePair.second));
    assert(nodeEdges_[nodePair.first].contains(edgeId));
    assert(nodeEdges_[nodePair.second].contains(edgeId));
    assert(nodePair.first < nodePair.second);

    nodeEdges_[nodePair.first].erase(edgeId);
    nodeEdges_[nodePair.second].erase(edgeId);
    edgeSet_.erase({nodePair.first, nodePair.second});
    edges_.erase(edgeId);
}

int Graph::GetNodeCount() const { return static_cast<int>(nodes_.size()); }

int Graph::GetEdgeCount() const { return static_cast<int>(edges_.size()); }

bool Graph::HasNodes() const { return !nodes_.empty(); }

bool Graph::NodeHasEdges(int nodeId) const
{
    assert(nodes_.contains(nodeId));

    return nodeEdges_.contains(nodeId) && !nodeEdges_.find(nodeId)->second.empty();
}

bool Graph::HasEdges() const { return !edges_.empty(); }

bool Graph::IsEmpty() const { return nodes_.empty() && edges_.empty(); }

void Graph::Clear()
{
    nodeEdges_.clear();
    edges_.clear();
    edgeSet_.clear();
    nodes_.clear();

    nextNodeId_ = 1;
    nextEdgeId_ = 1;
}

void Graph::ClearEdges()
{
    nodeEdges_.clear();
    edges_.clear();
    edgeSet_.clear();

    nextEdgeId_ = 1;
}
