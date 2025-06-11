#pragma once

#include <utility>
#include <map>
#include <vector>
#include <unordered_set>

#include "mirael_glm_ext.hpp"
#include "vec2.hpp"

class Graph {
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
	Node GetNode(int nodeId) const;
	std::vector<Node> GetNodes() const;
	void RepositionNode(int nodeId, glm::vec2 pos);
	void RemoveNode(int nodeId);

	int AddEdge(int nodeIdA, int nodeIdB);
	Edge GetEdge(int edgeId) const;
	std::vector<Edge> GetEdges() const;
	std::vector<Edge> GetEdges(int nodeId) const;
	void RemoveEdge(int edgeId);

private:
	int nextNodeId_ = 1;
	std::map<int, glm::vec2> nodes_;

	int nextEdgeId_ = 1;
	std::map<int, std::pair<int, int>> edges_; // edge id to node ids {a, b}

	std::map<int, std::unordered_set<int>> nodeEdges_; // node id to set of edge id, for both ends (has every edge twice)
};
