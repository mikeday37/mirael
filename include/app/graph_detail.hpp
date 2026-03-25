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
    using iterator = ValueIterator<typename std::map<int, typename GraphType::Node>::const_iterator>;

    NodeRange(const GraphType &g) noexcept : graph_(g) {}

    iterator begin() const noexcept { return iterator(graph_.nodes_.begin()); }
    iterator end() const noexcept { return iterator(graph_.nodes_.end()); }
    size_t size() const noexcept { return graph_.nodes_.size(); }

private:
    const GraphType &graph_;
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class EdgeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using iterator = ValueIterator<typename std::unordered_map<int, typename GraphType::Edge>::const_iterator>;

    EdgeRange(const GraphType &g) noexcept : graph_(g) {}

    iterator begin() const noexcept { return iterator(graph_.edges_.begin()); }
    iterator end() const noexcept { return iterator(graph_.edges_.end()); }
    size_t size() const noexcept { return graph_.edges_.size(); }

private:
    const GraphType &graph_;
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class NodeEdgeIterator
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using InnerIterator = std::unordered_set<int>::const_iterator;
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename GraphType::Edge;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = const value_type &;

    NodeEdgeIterator() noexcept = default;
    NodeEdgeIterator(const GraphType *graph, InnerIterator it) noexcept : graph_(graph), it_(it) {}

    reference operator*() const { return graph_->edges_.at(*it_); }
    pointer operator->() const { return &(graph_->edges_.at(*it_)); }

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
    const GraphType *graph_ = nullptr;
    InnerIterator it_{};
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class NodeEdgeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using iterator = NodeEdgeIterator<TType, TNodeData, TEdgeData>;

    NodeEdgeRange(const GraphType *graph, int nodeId)
        : graph_(graph), edges_(graph->nodeEdges_.contains(nodeId) ? graph->nodeEdges_.at(nodeId) : emptySet_)
    {
    }

    iterator begin() const noexcept { return iterator(graph_, edges_.begin()); }
    iterator end() const noexcept { return iterator(graph_, edges_.end()); }
    size_t size() const noexcept { return edges_.size(); }

private:
    const GraphType *graph_;
    const std::unordered_set<int> &edges_;
    inline static std::unordered_set<int> emptySet_;
};

}; // namespace GraphDetail
