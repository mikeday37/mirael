#pragma once

#include "app/utility.hpp"
#include "vec2.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct Empty {
};

enum class GraphType { Directed, Undirected };

template <GraphType TType, typename TNodeData, typename TEdgeData> class Graph
{
    static_assert(TType == GraphType::Directed || TType == GraphType::Undirected,
                  "TType must be either GraphType::Undirected or GraphType::Directed");
    static_assert(std::is_class_v<TNodeData>, "TNodeData must be a class or struct.");
    static_assert(std::is_class_v<TEdgeData>, "TEdgeData must be a class or struct.");

public:
    struct Node {
        int id;
        TNodeData data;
    };

    struct Edge {
        int id;
        int nodeIdA;
        int nodeIdB;
    };

    struct AddEdgeResult {
        int id;
        bool added; // true if it didn't already exist, false if it already existed
    };

    int AddNode(glm::vec2 pos); // always returns a new id
    bool ContainsNode(int nodeId) const;
    Node GetNode(int nodeId) const;
    std::vector<Node> GetNodes() const;
    void RepositionNode(int nodeId, glm::vec2 pos);
    void RemoveNode(int nodeId);

    AddEdgeResult AddEdge(int nodeIdA,
                          int nodeIdB); // returns a new id if added == true, or the existing id if added == false
    bool ContainsEdge(int edgeId) const;
    bool ContainsEdge(int nodeIdA, int nodeIdB) const;
    Edge GetEdge(int edgeId) const;
    std::vector<Edge> GetEdges() const;
    std::vector<Edge> GetEdges(int nodeId) const;
    void RemoveEdge(int edgeId);

    int GetNodeCount() const;
    int GetEdgeCount() const;

    bool HasNodes() const;
    bool NodeHasEdges(int nodeId) const;
    bool HasEdges() const;
    bool IsEmpty() const;

    void Clear();
    void ClearEdges();

private:
    std::pair<int, int> CanonicalEdge(int a, int b) const;

    int nextId_ = 1;
    std::map<int, TNodeData> nodes_;

    std::unordered_map<int, std::pair<int, int>> edges_;             // edge id -> node ids {a, b}
    std::unordered_map<std::pair<int, int>, int, PairHash> edgeMap_; // node ids {a, b} -> edge Id
    std::unordered_map<int, std::unordered_set<int>>
        nodeEdges_; // node id to set of edge id, for both ends (has every edge twice) // TODO: check & update - this
                    // makes no sense
};

template <typename TNodeData = Empty, typename TEdgeData = Empty>
using DirectedGraph = Graph<GraphType::Directed, TNodeData, TEdgeData>;

template <typename TNodeData = Empty, typename TEdgeData = Empty>
using UndirectedGraph = Graph<GraphType::Undirected, TNodeData, TEdgeData>;

#include "app/graph.tpp"