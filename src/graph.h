#pragma once

#include <string>
#include <string_view>

#include "data.h"

namespace Mirael
{

class Graph
{
public:
    // forbid copy, allow move
    Graph()                         = default;
    Graph(const Graph &)            = delete;
    Graph &operator=(const Graph &) = delete;
    Graph(Graph &&)                 = default;
    Graph &operator=(Graph &&)      = default;

    enum class ChangeImpact {
        Name, // used if the name changed (may include other changes)
        Other // used only if the name did not change
    };
    using ChangeCallback = std::function<void(ChangeImpact)>;
    ChangeCallback onModified;

    void rename(std::string newName);
    std::string_view getName() const { return name; }

    GraphData toData() const;
    static Graph fromData(const GraphData &data);

private:
    std::string name{"Graph"};
    bool visible{true};
};

}; // namespace Mirael