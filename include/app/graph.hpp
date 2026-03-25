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

    template <typename MapIterator> class ValueIterator
    {
    private:
        MapIterator it_;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename MapIterator::value_type::second_type;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        ValueIterator(MapIterator it) : it_(it) {}

        reference operator*() const { return it_->second; }
        pointer operator->() const { return &(it_->second); }

        ValueIterator &operator++()
        {
            ++it_;
            return *this;
        }
        ValueIterator operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const ValueIterator &other) const { return it_ == other.it_; }
        bool operator!=(const ValueIterator &other) const { return it_ != other.it_; }
    };

    class NodeRange
    {
    private:
        const Graph *graph_;

    public:
        using iterator = ValueIterator<typename std::map<int, Node>::const_iterator>;

        NodeRange(const Graph *g) : graph_(g) {}

        iterator begin() const { return iterator(graph_->nodes_.begin()); }
        iterator end() const { return iterator(graph_->nodes_.end()); }
        size_t size() const { return graph_->nodes_.size(); }
    };

    template <typename... Args> int AddNode(Args &&...args); // always returns a new node id
    TNodeData &NodeData(int nodeId);
    const TNodeData &NodeData(int nodeId) const;

    bool ContainsNode(int nodeId) const;
    const Node &GetNode(int nodeId) const;
    NodeRange Nodes() const { return NodeRange(this); }
    void RemoveNode(int nodeId);

    template <typename... Args>
    AddEdgeResult AddEdge(int nodeIdA, int nodeIdB,
                          Args &&...args); // returns a new id if added == true, or the existing id if added == false
    TEdgeData &EdgeData(int edgeId);
    const TEdgeData &EdgeData(int edgeId) const;

    bool ContainsEdge(int edgeId) const;
    bool ContainsEdge(int nodeIdA, int nodeIdB) const;
    const Edge &GetEdge(int edgeId) const;
    const Edge &GetEdge(int nodeIdA, int nodeIdB) const;
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
    std::map<int, Node> nodes_;
    std::unordered_map<int, Edge> edges_;
    std::unordered_map<std::pair<int, int>, int, PairHash> edgeMap_; // node ids {a, b} -> edge Id
    std::unordered_map<int, std::unordered_set<int>> nodeEdges_;     // node id -> set of adjacent edge ids
};

template <typename TNodeData = Empty, typename TEdgeData = Empty>
using DirectedGraph = Graph<GraphType::Directed, TNodeData, TEdgeData>;

template <typename TNodeData = Empty, typename TEdgeData = Empty>
using UndirectedGraph = Graph<GraphType::Undirected, TNodeData, TEdgeData>;

#include "app/graph.tpp"