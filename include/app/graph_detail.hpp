#pragma once

template <GraphType TType, typename TNodeData, typename TEdgeData> class Graph;

namespace GraphDetail
{

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

template <GraphType TType, typename TNodeData, typename TEdgeData> class NodeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using iterator = ValueIterator<typename std::map<int, typename GraphType::Node>::const_iterator>;

    NodeRange(const GraphType *g) : graph_(g) {}

    iterator begin() const { return iterator(graph_->nodes_.begin()); }
    iterator end() const { return iterator(graph_->nodes_.end()); }
    size_t size() const { return graph_->nodes_.size(); }

private:
    const GraphType *graph_;
};

template <GraphType TType, typename TNodeData, typename TEdgeData> class EdgeRange
{
public:
    using GraphType = Graph<TType, TNodeData, TEdgeData>;
    using iterator = ValueIterator<typename std::unordered_map<int, typename GraphType::Edge>::const_iterator>;

    EdgeRange(const GraphType *g) : graph_(g) {}

    iterator begin() const { return iterator(graph_->edges_.begin()); }
    iterator end() const { return iterator(graph_->edges_.end()); }
    size_t size() const { return graph_->edges_.size(); }

private:
    const GraphType *graph_;
};

}; // namespace GraphDetail
