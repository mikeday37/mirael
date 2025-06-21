#pragma once

#include "app/utility.hpp"
#include "vec2.hpp"
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

class Graph
{
public:
    struct Node {
        int id;
        glm::vec2 pos;
    };

    struct Edge {
        int id;
        int nodeIdA;
        int nodeIdB;
    };

    int AddNode(glm::vec2 pos);
    bool ContainsNode(int nodeId) const;
    Node GetNode(int nodeId) const;
    std::vector<Node> GetNodes() const;
    void RepositionNode(int nodeId, glm::vec2 pos);
    void RemoveNode(int nodeId);

    int AddEdge(int nodeIdA, int nodeIdB);
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
    int nextNodeId_ = 1;
    std::map<int, glm::vec2> nodes_;

    int nextEdgeId_ = 1;
    std::map<int, std::pair<int, int>> edges_;                  // edge id to node ids {a, b}
    std::unordered_set<std::pair<int, int>, PairHash> edgeSet_; // contains {a,b} where a < b for every edge
    std::map<int, std::unordered_set<int>>
        nodeEdges_; // node id to set of edge id, for both ends (has every edge twice)
};
