#include "app_pch.hpp"

#include "app/graph.hpp"

#include <limits>

int Graph::AddNode(glm::vec2 pos)
{
	assert(nextNodeId_ < std::numeric_limits<int>::max());

    int nodeId = nextNodeId_++;
	nodes_[nodeId] = pos;
	nodeEdges_[nodeId];

	return nodeId;
}

std::vector<Graph::Node> Graph::GetNodes() const
{
	std::vector<Graph::Node> nodes;
	nodes.reserve(nodes_.size());

	for (const auto &[id, pos] : nodes_) {
		nodes.emplace_back(id, pos);
	}

    return nodes;
}

void Graph::RemoveNode(int nodeId)
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
		edges_.erase(edgeId);
	}

	// finally erase the edge set itself for this node, and erase the node.
	nodeEdges_.erase(nodeId);
	nodes_.erase(nodeId);
}

int Graph::AddEdge(int nodeIdA, int nodeIdB)
{
	assert(nodes_.contains(nodeIdA));
	assert(nodes_.contains(nodeIdB));

	int edgeId = nextEdgeId_++;

	edges_[edgeId] = {nodeIdA, nodeIdB};
	nodeEdges_[nodeIdA].emplace(edgeId);
	nodeEdges_[nodeIdB].emplace(edgeId);

	return edgeId;
}

std::vector<Graph::Edge> Graph::GetEdges() const
{
	std::vector<Graph::Edge> edges;
	edges.reserve(edges_.size());

	for (const auto &[id, nodes] : edges_) {
		edges.emplace_back(id, nodes.first, nodes.second);
	}

    return edges;
}

void Graph::RemoveEdge(int edgeId)
{
	assert(edges_.contains(edgeId));

	const auto &nodePair = edges_[edgeId];

	assert(nodes_.contains(nodePair.first));
	assert(nodes_.contains(nodePair.second));
	assert(nodeEdges_[nodePair.first].contains(edgeId));
	assert(nodeEdges_[nodePair.second].contains(edgeId));

	nodeEdges_[nodePair.first].erase(edgeId);
	nodeEdges_[nodePair.second].erase(edgeId);
	edges_.erase(edgeId);
}
