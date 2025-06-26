#include "app/graph.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ranges>

bool CheckUndirectedEdgeMatch(UndirectedGraph<>::Edge edge, int nodeIdA, int nodeIdB)
{
    return (edge.nodeIdA == nodeIdA && edge.nodeIdB == nodeIdB) || (edge.nodeIdA == nodeIdB && edge.nodeIdB == nodeIdA);
}

constexpr glm::vec2 k_node1_pos{1, -111};
constexpr glm::vec2 k_node2_pos{2, -2222};

TEST_CASE("Undirected: Empty graph returns expected values")
{
    UndirectedGraph<> g;

    REQUIRE(g.IsEmpty());
    REQUIRE(!g.HasNodes());
    REQUIRE(!g.HasEdges());

    REQUIRE(g.GetNodeCount() == 0);
    REQUIRE(g.GetEdgeCount() == 0);

    REQUIRE(g.GetNodes().size() == 0);
    REQUIRE(g.GetEdges().size() == 0);

    REQUIRE(g.ContainsNode(1) == false);
    REQUIRE(g.ContainsEdge(1) == false);
    REQUIRE(g.ContainsEdge(1, 2) == false);
}

TEST_CASE("Undirected: Basic graph creation and manipulation")
{
    UndirectedGraph<> g;

    // now for the objects we care about
    auto nodeId1 = g.AddNode(k_node1_pos);
    auto nodeId2 = g.AddNode(k_node2_pos);
    auto edgeId = g.AddEdge(nodeId1, nodeId2).id;

    auto node1 = g.GetNode(nodeId1);
    auto node2 = g.GetNode(nodeId2);
    auto edge = g.GetEdge(edgeId);

    SECTION("id separation")
    {
        REQUIRE(nodeId1 != nodeId2);
        REQUIRE(edgeId != nodeId1);
        REQUIRE(edgeId != nodeId2);
    }

    SECTION("ids are not zero")
    {
        REQUIRE(nodeId1 != 0);
        REQUIRE(nodeId2 != 0);
        REQUIRE(edgeId != 0);
    }

    SECTION("ids for the same type of object do not conflict")
    {
        auto nodeId3 = g.AddNode({0, 0});
        auto edgeId2 = g.AddEdge(nodeId1, nodeId3).id;

        REQUIRE(nodeId1 != nodeId2);
        REQUIRE(nodeId2 != nodeId3);
        REQUIRE(nodeId1 != nodeId3);

        REQUIRE(edgeId != edgeId2);
    }

    SECTION("inspection at initial state")
    {
        REQUIRE(!g.IsEmpty());
        REQUIRE(g.HasNodes());
        REQUIRE(g.HasEdges());

        REQUIRE(g.GetNodeCount() == 2);
        REQUIRE(g.GetEdgeCount() == 1);

        REQUIRE(g.GetNodes().size() == 2);
        REQUIRE(g.GetEdges().size() == 1);

        REQUIRE(g.ContainsNode(nodeId1));
        REQUIRE(g.ContainsNode(nodeId2));

        REQUIRE(g.ContainsEdge(edgeId));
        REQUIRE(g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(node1.id == nodeId1);
        REQUIRE(node2.id == nodeId2);
        REQUIRE(edge.id == edgeId);

        REQUIRE(node1.pos != node2.pos);
        REQUIRE(node1.pos == k_node1_pos);
        REQUIRE(node2.pos == k_node2_pos);

        REQUIRE(CheckUndirectedEdgeMatch(edge, nodeId1, nodeId2));

        REQUIRE(g.NodeHasEdges(nodeId1));
        REQUIRE(g.NodeHasEdges(nodeId2));
        REQUIRE(g.GetEdges(nodeId1).size() == 1);
        REQUIRE(g.GetEdges(nodeId2).size() == 1);
        REQUIRE(CheckUndirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId1, nodeId2));
        REQUIRE(CheckUndirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId1, nodeId2));
        REQUIRE(g.GetEdges(nodeId1)[0].id == edgeId);
        REQUIRE(g.GetEdges(nodeId2)[0].id == edgeId);
    }

    SECTION("inspection after removing the edge")
    {
        g.RemoveEdge(edgeId);

        REQUIRE(!g.IsEmpty());
        REQUIRE(g.HasNodes());
        REQUIRE(!g.HasEdges());

        REQUIRE(g.GetNodeCount() == 2);
        REQUIRE(g.GetEdgeCount() == 0);

        REQUIRE(g.GetNodes().size() == 2);
        REQUIRE(g.GetEdges().size() == 0);

        REQUIRE(g.ContainsNode(nodeId1));
        REQUIRE(g.ContainsNode(nodeId2));

        REQUIRE(!g.ContainsEdge(edgeId));
        REQUIRE(!g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(!g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(!g.NodeHasEdges(nodeId1));
        REQUIRE(!g.NodeHasEdges(nodeId2));
        REQUIRE(g.GetEdges(nodeId1).size() == 0);
        REQUIRE(g.GetEdges(nodeId2).size() == 0);
    }

    SECTION("inspection after clear edges")
    {
        g.ClearEdges();

        REQUIRE(!g.IsEmpty());
        REQUIRE(g.HasNodes());
        REQUIRE(!g.HasEdges());

        REQUIRE(g.GetNodeCount() == 2);
        REQUIRE(g.GetEdgeCount() == 0);

        REQUIRE(g.GetNodes().size() == 2);
        REQUIRE(g.GetEdges().size() == 0);

        REQUIRE(g.ContainsNode(nodeId1));
        REQUIRE(g.ContainsNode(nodeId2));

        REQUIRE(!g.ContainsEdge(edgeId));
        REQUIRE(!g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(!g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(!g.NodeHasEdges(nodeId1));
        REQUIRE(!g.NodeHasEdges(nodeId2));
        REQUIRE(g.GetEdges(nodeId1).size() == 0);
        REQUIRE(g.GetEdges(nodeId2).size() == 0);
    }

    SECTION("inspection after clear all")
    {
        g.Clear();

        REQUIRE(g.IsEmpty());
        REQUIRE(!g.HasNodes());
        REQUIRE(!g.HasEdges());

        REQUIRE(g.GetNodeCount() == 0);
        REQUIRE(g.GetEdgeCount() == 0);

        REQUIRE(g.GetNodes().size() == 0);
        REQUIRE(g.GetEdges().size() == 0);

        REQUIRE(!g.ContainsNode(nodeId1));
        REQUIRE(!g.ContainsNode(nodeId2));

        REQUIRE(!g.ContainsEdge(edgeId));
        REQUIRE(!g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(!g.ContainsEdge(nodeId2, nodeId1));
    }

    SECTION("inspection after removing node 1")
    {
        g.RemoveNode(nodeId1);

        REQUIRE(!g.IsEmpty());
        REQUIRE(g.HasNodes());
        REQUIRE(!g.HasEdges());

        REQUIRE(g.GetNodeCount() == 1);
        REQUIRE(g.GetEdgeCount() == 0);

        REQUIRE(g.GetNodes().size() == 1);
        REQUIRE(g.GetEdges().size() == 0);

        REQUIRE(g.GetNodes()[0].id == nodeId2);
        REQUIRE(g.GetNodes()[0].pos == k_node2_pos);

        REQUIRE(!g.ContainsEdge(edgeId));
        REQUIRE(!g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(!g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(!g.ContainsNode(nodeId1));
        REQUIRE(g.ContainsNode(nodeId2));

        REQUIRE(!g.NodeHasEdges(nodeId2));
        REQUIRE(g.GetEdges(nodeId2).size() == 0);
    }

    SECTION("inspection after removing node 2")
    {
        g.RemoveNode(nodeId2);

        REQUIRE(!g.IsEmpty());
        REQUIRE(g.HasNodes());
        REQUIRE(!g.HasEdges());

        REQUIRE(g.GetNodeCount() == 1);
        REQUIRE(g.GetEdgeCount() == 0);

        REQUIRE(g.GetNodes().size() == 1);
        REQUIRE(g.GetEdges().size() == 0);

        REQUIRE(g.GetNodes()[0].id == nodeId1);
        REQUIRE(g.GetNodes()[0].pos == k_node1_pos);

        REQUIRE(!g.ContainsEdge(edgeId));
        REQUIRE(!g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(!g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(g.ContainsNode(nodeId1));
        REQUIRE(!g.ContainsNode(nodeId2));

        REQUIRE(!g.NodeHasEdges(nodeId1));
        REQUIRE(g.GetEdges(nodeId1).size() == 0);
    }

    SECTION("inspection after removing both nodes")
    {
        g.RemoveNode(nodeId1);
        g.RemoveNode(nodeId2);

        REQUIRE(g.IsEmpty());
        REQUIRE(!g.HasNodes());
        REQUIRE(!g.HasEdges());

        REQUIRE(g.GetNodeCount() == 0);
        REQUIRE(g.GetEdgeCount() == 0);

        REQUIRE(g.GetNodes().size() == 0);
        REQUIRE(g.GetEdges().size() == 0);

        REQUIRE(!g.ContainsEdge(edgeId));
        REQUIRE(!g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(!g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(!g.ContainsNode(nodeId1));
        REQUIRE(!g.ContainsNode(nodeId2));
    }

    SECTION("inspection after removing the edge and adding an equivalent")
    {
        g.RemoveEdge(edgeId);
        auto newEdgeId = g.AddEdge(nodeId2, nodeId1).id;
        auto newEdge = g.GetEdge(newEdgeId);

        REQUIRE(!g.IsEmpty());
        REQUIRE(g.HasNodes());
        REQUIRE(g.HasEdges());

        REQUIRE(g.GetNodeCount() == 2);
        REQUIRE(g.GetEdgeCount() == 1);

        REQUIRE(g.GetNodes().size() == 2);
        REQUIRE(g.GetEdges().size() == 1);

        REQUIRE(g.ContainsEdge(newEdgeId));
        REQUIRE(g.ContainsEdge(nodeId1, nodeId2));
        REQUIRE(g.ContainsEdge(nodeId2, nodeId1));

        REQUIRE(g.ContainsNode(nodeId1));
        REQUIRE(g.ContainsNode(nodeId2));

        REQUIRE(g.NodeHasEdges(nodeId1));
        REQUIRE(g.NodeHasEdges(nodeId2));

        REQUIRE(g.GetEdges(nodeId1).size() == 1);
        REQUIRE(g.GetEdges(nodeId2).size() == 1);
        REQUIRE(CheckUndirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId1, nodeId2));
        REQUIRE(CheckUndirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId1, nodeId2));
        REQUIRE(g.GetEdges(nodeId1)[0].id == newEdgeId);
        REQUIRE(g.GetEdges(nodeId2)[0].id == newEdgeId);

        REQUIRE(CheckUndirectedEdgeMatch(newEdge, nodeId1, nodeId2));
        REQUIRE(newEdge.id == newEdgeId);
        REQUIRE(newEdge.id == g.GetEdge(newEdgeId).id);
    }
}
