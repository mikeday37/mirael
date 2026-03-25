#pragma once

#include "app/utility.hpp"
#include "vec2.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

enum class GraphType { Directed, Undirected };

#include "graph_detail.hpp"

template <GraphType TType, typename TNodeData, typename TEdgeData> class Graph
{
    static_assert(TType == GraphType::Directed || TType == GraphType::Undirected,
                  "TType must be either GraphType::Undirected or GraphType::Directed");

public:
    struct Node {
        int id;
        TNodeData data;
    };

    struct Edge {
        int id;
        int nodeIdA;
        int nodeIdB;
        TEdgeData data;
    };

    struct AddEdgeResult {
        int id;
        bool added; // true if it didn't already exist, false if it already existed
    };

    using NodeRange = GraphDetail::NodeRange<TType, TNodeData, TEdgeData>;
    using EdgeRange = GraphDetail::EdgeRange<TType, TNodeData, TEdgeData>;
    using NodeEdgeRange = GraphDetail::NodeEdgeRange<TType, TNodeData, TEdgeData>;

    template <typename... Args> int AddNode(Args &&...args); // always returns a new node id
    TNodeData &NodeData(int nodeId);
    const TNodeData &NodeData(int nodeId) const;

    bool ContainsNode(int nodeId) const noexcept;
    const Node &GetNode(int nodeId) const;
    NodeRange Nodes() const noexcept { return NodeRange(nodes_); }
    void RemoveNode(int nodeId);

    template <typename... Args>
    AddEdgeResult AddEdge(int nodeIdA, int nodeIdB,
                          Args &&...args); // returns a new id if added == true, or the existing id if added == false
    TEdgeData &EdgeData(int edgeId);
    const TEdgeData &EdgeData(int edgeId) const;

    bool ContainsEdge(int edgeId) const noexcept;
    bool ContainsEdge(int nodeIdA, int nodeIdB) const noexcept;
    const Edge &GetEdge(int edgeId) const;
    const Edge &GetEdge(int nodeIdA, int nodeIdB) const;
    EdgeRange Edges() const noexcept { return EdgeRange(edges_); }
    NodeEdgeRange NodeEdges(int nodeId) { return NodeEdgeRange(*this, nodeId); }
    void RemoveEdge(int edgeId);

    int GetNodeCount() const noexcept;
    int GetEdgeCount() const noexcept;

    bool HasNodes() const noexcept;
    bool NodeHasEdges(int nodeId) const noexcept;
    bool HasEdges() const noexcept;
    bool IsEmpty() const noexcept;

    void Clear() noexcept;
    void ClearEdges() noexcept;

private:
    std::pair<int, int> CanonicalEdge(int a, int b) const noexcept;

    int nextId_ = 1;
    std::map<int, Node> nodes_;
    std::unordered_map<int, Edge> edges_;
    std::unordered_map<std::pair<int, int>, int, PairHash> edgeMap_; // node ids {a, b} -> edge Id
    std::unordered_map<int, std::unordered_set<int>> nodeEdges_;     // node id -> set of adjacent edge ids

    friend NodeRange;
    friend EdgeRange;
    friend GraphDetail::NodeEdgeIterator<TType, TNodeData, TEdgeData>;
    friend NodeEdgeRange;
};

template <typename TNodeData = std::monostate, typename TEdgeData = std::monostate>
using DirectedGraph = Graph<GraphType::Directed, TNodeData, TEdgeData>;

template <typename TNodeData = std::monostate, typename TEdgeData = std::monostate>
using UndirectedGraph = Graph<GraphType::Undirected, TNodeData, TEdgeData>;

#include "app/graph.tpp"