#pragma once

#include "app/graph.hpp"
#include "vec2.hpp"
#include <limits>
#include <map>
#include <vector>

template <GraphType TType, typename TNodeData, typename TEdgeData>
inline std::pair<int, int> Graph<TType, TNodeData, TEdgeData>::CanonicalEdge(int a, int b) const
{
    if constexpr (TType == GraphType::Undirected) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    } else {
        return std::make_pair(a, b);
    }
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
template <typename... Args>
int Graph<TType, TNodeData, TEdgeData>::AddNode(Args &&...args)
{
    assert(nextId_ < std::numeric_limits<int>::max());

    int nodeId = nextId_++;
    nodes_.emplace(std::piecewise_construct, std::forward_as_tuple(nodeId),
                   std::forward_as_tuple(std::forward<Args>(args)...));
    nodeEdges_[nodeId];

    return nodeId;
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
int Graph<TType, TNodeData, TEdgeData>::AddNode(TNodeData &data)
{
    assert(nextId_ < std::numeric_limits<int>::max());

    int nodeId = nextId_++;
    nodes_[nodeId] = data;
    nodeEdges_[nodeId];

    return nodeId;
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
TNodeData &Graph<TType, TNodeData, TEdgeData>::NodeData(int nodeId)
{
    assert(nodes_.contains(nodeId));
    return nodes_.at(nodeId);
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::ContainsNode(int nodeId) const
{
    return nodes_.contains(nodeId);
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
Graph<TType, TNodeData, TEdgeData>::Node Graph<TType, TNodeData, TEdgeData>::GetNode(int nodeId) const
{
    assert(nodes_.contains(nodeId));

    auto it = nodes_.find(nodeId);

    return {nodeId, it->second};
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
std::vector<typename Graph<TType, TNodeData, TEdgeData>::Node> Graph<TType, TNodeData, TEdgeData>::GetNodes() const
{
    std::vector<Graph<TType, TNodeData, TEdgeData>::Node> nodes;
    nodes.reserve(nodes_.size());

    for (const auto &[id, pos] : nodes_) {
        nodes.emplace_back(id, pos);
    }

    return nodes;
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
void Graph<TType, TNodeData, TEdgeData>::RepositionNode(int nodeId, glm::vec2 pos)
{
    assert(nodes_.contains(nodeId));

    auto it = nodes_.find(nodeId);

    it->second.x = pos.x;
    it->second.y = pos.y;
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
void Graph<TType, TNodeData, TEdgeData>::RemoveNode(int nodeId)
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
        edgeMap_.erase(CanonicalEdge(nodeIdA, nodeIdB));
        edges_.erase(edgeId);
    }

    // finally erase the edge set itself for this node, and erase the node.
    nodeEdges_.erase(nodeId);
    nodes_.erase(nodeId);
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
Graph<TType, TNodeData, TEdgeData>::AddEdgeResult Graph<TType, TNodeData, TEdgeData>::AddEdge(int nodeIdA, int nodeIdB)
{
    assert(nextId_ < std::numeric_limits<int>::max());
    assert(nodeIdA != nodeIdB);
    assert(nodes_.contains(nodeIdA));
    assert(nodes_.contains(nodeIdB));

    auto edgeKey = CanonicalEdge(nodeIdA, nodeIdB);

    auto it = edgeMap_.find(edgeKey);
    if (it != edgeMap_.end()) {
        return {it->second, false};
    }

    int edgeId = nextId_++;

    edges_[edgeId] = edgeKey;
    nodeEdges_[nodeIdA].emplace(edgeId);
    nodeEdges_[nodeIdB].emplace(edgeId);
    edgeMap_.emplace(edgeKey, edgeId);

    return {edgeId, true};
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::ContainsEdge(int edgeId) const
{
    return edges_.contains(edgeId);
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::ContainsEdge(int nodeIdA, int nodeIdB) const
{
    assert(nodeIdA != nodeIdB);

    return edgeMap_.contains(CanonicalEdge(nodeIdA, nodeIdB));
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
Graph<TType, TNodeData, TEdgeData>::Edge Graph<TType, TNodeData, TEdgeData>::GetEdge(int edgeId) const
{
    assert(edges_.contains(edgeId));

    auto it = edges_.find(edgeId);

    return {edgeId, it->second.first, it->second.second};
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
std::vector<typename Graph<TType, TNodeData, TEdgeData>::Edge> Graph<TType, TNodeData, TEdgeData>::GetEdges() const
{
    std::vector<Graph<TType, TNodeData, TEdgeData>::Edge> edges;
    edges.reserve(edges_.size());

    for (const auto &[id, nodes] : edges_) {
        edges.emplace_back(id, nodes.first, nodes.second);
    }

    return edges;
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
std::vector<typename Graph<TType, TNodeData, TEdgeData>::Edge>
Graph<TType, TNodeData, TEdgeData>::GetEdges(int nodeId) const
{
    assert(nodes_.contains(nodeId));

    std::vector<Graph<TType, TNodeData, TEdgeData>::Edge> edges;

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

template <GraphType TType, typename TNodeData, typename TEdgeData>
void Graph<TType, TNodeData, TEdgeData>::RemoveEdge(int edgeId)
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
    edgeMap_.erase({nodePair.first, nodePair.second});
    edges_.erase(edgeId);
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
int Graph<TType, TNodeData, TEdgeData>::GetNodeCount() const
{
    return static_cast<int>(nodes_.size());
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
int Graph<TType, TNodeData, TEdgeData>::GetEdgeCount() const
{
    return static_cast<int>(edges_.size());
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::HasNodes() const
{
    return !nodes_.empty();
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::NodeHasEdges(int nodeId) const
{
    assert(nodes_.contains(nodeId));

    return nodeEdges_.contains(nodeId) && !nodeEdges_.find(nodeId)->second.empty();
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::HasEdges() const
{
    return !edges_.empty();
}

template <GraphType TType, typename TNodeData, typename TEdgeData>
bool Graph<TType, TNodeData, TEdgeData>::IsEmpty() const
{
    return nodes_.empty() && edges_.empty();
}

template <GraphType TType, typename TNodeData, typename TEdgeData> void Graph<TType, TNodeData, TEdgeData>::Clear()
{
    nodeEdges_.clear();
    edges_.clear();
    edgeMap_.clear();
    nodes_.clear();

    nextId_ = 1;
}

template <GraphType TType, typename TNodeData, typename TEdgeData> void Graph<TType, TNodeData, TEdgeData>::ClearEdges()
{
    nodeEdges_.clear();
    edges_.clear();
    edgeMap_.clear();

    if (nodes_.empty()) {
        nextId_ = 1;
    }
}
