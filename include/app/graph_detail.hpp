#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>

template <GraphType TType, typename TNodeData, typename TEdgeData> class Graph;

namespace GraphDetail
{

template <typename MapIterator> class ValueIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename MapIterator::value_type::second_type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = const value_type &;

    ValueIterator() noexcept = default;
    ValueIterator(MapIterator it) noexcept : it_(it) {}

    reference operator*() const noexcept { return it_->second; }
    pointer operator->() const noexcept { return &(it_->second); }

    ValueIterator &operator++() noexcept
    {
        ++it_;
        return *this;
    }
    ValueIterator operator++(int) noexcept
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const ValueIterator &other) const noexcept { return it_ == other.it_; }
    bool operator!=(const ValueIterator &other) const noexcept { return it_ != other.it_; }

private:
    MapIterator it_{};
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class NodeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using NodeMapType = std::map<int, typename GraphType::Node>;
    using iterator = ValueIterator<typename NodeMapType::const_iterator>;

    NodeRange(const NodeMapType &nodes) noexcept : nodes_(nodes) {}

    iterator begin() const noexcept { return iterator(nodes_.begin()); }
    iterator end() const noexcept { return iterator(nodes_.end()); }
    size_t size() const noexcept { return nodes_.size(); }

private:
    const NodeMapType &nodes_;
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class EdgeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using EdgeMapType = std::unordered_map<int, typename GraphType::Edge>;
    using iterator = ValueIterator<typename EdgeMapType::const_iterator>;

    EdgeRange(const EdgeMapType &edges) noexcept : edges_(edges) {}

    iterator begin() const noexcept { return iterator(edges_.begin()); }
    iterator end() const noexcept { return iterator(edges_.end()); }
    size_t size() const noexcept { return edges_.size(); }

private:
    const EdgeMapType &edges_;
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class NodeEdgeIterator
{
    // This iterator returns only the edges for a particular node.
    // The allEdges map is used to look up the edges from the ids provided by the InnerIterator.

public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using EdgeMapType = std::unordered_map<int, typename GraphType::Edge>;
    using InnerIterator = std::unordered_set<int>::const_iterator;
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename GraphType::Edge;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = const value_type &;

    NodeEdgeIterator() noexcept = default;
    NodeEdgeIterator(const EdgeMapType *allEdges, InnerIterator it) noexcept : allEdges_(allEdges), it_(it) {}

    reference operator*() const { return allEdges_->at(*it_); }
    pointer operator->() const { return &allEdges_->at(*it_); }

    NodeEdgeIterator &operator++() noexcept
    {
        ++it_;
        return *this;
    }
    NodeEdgeIterator operator++(int) noexcept
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const NodeEdgeIterator &other) const noexcept { return it_ == other.it_; }
    bool operator!=(const NodeEdgeIterator &other) const noexcept { return it_ != other.it_; }

private:
    const EdgeMapType *allEdges_ = nullptr;
    InnerIterator it_{};
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class NodeEdgeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using EdgeMapType = std::unordered_map<int, typename GraphType::Edge>;
    using EdgeIdSetType = std::unordered_set<int>;
    using iterator = NodeEdgeIterator<TType, TNodeData, TEdgeData>;

private:
    static const EdgeIdSetType &GetMyEdges(const GraphType &graph, int nodeId) noexcept
    {
        auto it = graph.nodeEdges_.find(nodeId);
        return (it != graph.nodeEdges_.end()) ? it->second : emptySet_;
    }

public:
    NodeEdgeRange(const GraphType &graph, int nodeId) noexcept
        : allEdges_(graph.edges_), myEdgeIds_(GetMyEdges(graph, nodeId))
    {
    }

    iterator begin() const noexcept { return iterator(&allEdges_, myEdgeIds_.begin()); }
    iterator end() const noexcept { return iterator(&allEdges_, myEdgeIds_.end()); }
    size_t size() const noexcept { return myEdgeIds_.size(); }

private:
    const EdgeMapType &allEdges_;
    const EdgeIdSetType &myEdgeIds_;
    inline static std::unordered_set<int> emptySet_;
};

}; // namespace GraphDetail
