#include "app/graph.hpp"

#include "app/applet_graph_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <ranges>

bool CheckDirectedEdgeMatch(FractalAppletGraph::Edge edge, int nodeIdA, int nodeIdB)
{
    return edge.nodeIdA == nodeIdA && edge.nodeIdB == nodeIdB;
}

bool EdgeVectorContainsEdge(std::vector<typename FractalAppletGraph::Edge> edgeVector, int nodeIdA, int nodeIdB)
{
    for (const auto &edge : edgeVector) {
        if (CheckDirectedEdgeMatch(edge, nodeIdA, nodeIdB)) {
            return true;
        }
    }

    return false;
}

bool EdgeVectorContainsEdgeId(std::vector<typename FractalAppletGraph::Edge> edgeVector, int edgeId)
{
    for (const auto &edge : edgeVector) {
        if (edge.id == edgeId) {
            return true;
        }
    }

    return false;
}

constexpr glm::vec2 k_node1_pos{1, -111};
constexpr glm::vec2 k_node2_pos{2, -2222};

TEST_CASE("Directed: Empty graph returns expected values")
{
    FractalAppletGraph g;

    CHECK(g.IsEmpty());
    CHECK(!g.HasNodes());
    CHECK(!g.HasEdges());

    CHECK(g.GetNodeCount() == 0);
    CHECK(g.GetEdgeCount() == 0);

    CHECK(g.GetNodes().size() == 0);
    CHECK(g.GetEdges().size() == 0);

    CHECK(g.ContainsNode(1) == false);
    CHECK(g.ContainsEdge(1) == false);
    CHECK(g.ContainsEdge(1, 2) == false);
}

TEST_CASE("Directed: Basic graph creation and manipulation")
{
    FractalAppletGraph g;

    // now for the objects we care about
    auto nodeId1 = g.AddNode(k_node1_pos);
    auto nodeId2 = g.AddNode(k_node2_pos);
    auto edgeId = g.AddEdge(nodeId1, nodeId2).id;

    auto node1 = g.GetNode(nodeId1);
    auto node2 = g.GetNode(nodeId2);
    auto edge = g.GetEdge(edgeId);

    SECTION("id separation")
    {
        CHECK(nodeId1 != nodeId2);
        CHECK(edgeId != nodeId1);
        CHECK(edgeId != nodeId2);
    }

    SECTION("ids are not zero")
    {
        CHECK(nodeId1 != 0);
        CHECK(nodeId2 != 0);
        CHECK(edgeId != 0);
    }

    SECTION("ids for the same type of object do not conflict")
    {
        auto nodeId3 = g.AddNode({0, 0});
        auto edgeId2 = g.AddEdge(nodeId1, nodeId3).id;

        CHECK(nodeId1 != nodeId2);
        CHECK(nodeId2 != nodeId3);
        CHECK(nodeId1 != nodeId3);

        CHECK(edgeId != edgeId2);
    }

    SECTION("inspection at initial state")
    {
        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(g.HasEdges());

        CHECK(g.GetNodeCount() == 2);
        CHECK(g.GetEdgeCount() == 1);

        CHECK(g.GetNodes().size() == 2);
        CHECK(g.GetEdges().size() == 1);

        CHECK(g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(g.ContainsEdge(edgeId));
        CHECK(g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(node1.id == nodeId1);
        CHECK(node2.id == nodeId2);
        CHECK(edge.id == edgeId);

        CHECK(node1.pos != node2.pos);
        CHECK(node1.pos == k_node1_pos);
        CHECK(node2.pos == k_node2_pos);

        CHECK(CheckDirectedEdgeMatch(edge, nodeId1, nodeId2));

        CHECK(g.NodeHasEdges(nodeId1));
        CHECK(g.NodeHasEdges(nodeId2));
        CHECK(g.GetEdges(nodeId1).size() == 1);
        CHECK(g.GetEdges(nodeId2).size() == 1);
        CHECK(CheckDirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId1, nodeId2));
        CHECK(CheckDirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId1, nodeId2));
        CHECK(g.GetEdges(nodeId1)[0].id == edgeId);
        CHECK(g.GetEdges(nodeId2)[0].id == edgeId);
    }

    SECTION("inspection after adding the opposite")
    {
        auto edgeId2 = g.AddEdge(nodeId2, nodeId1).id;
        auto edge2 = g.GetEdge(edgeId2);

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(g.HasEdges());

        CHECK(g.GetNodeCount() == 2);
        CHECK(g.GetEdgeCount() == 2);

        CHECK(g.GetNodes().size() == 2);
        CHECK(g.GetEdges().size() == 2);

        CHECK(g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(g.ContainsEdge(edgeId));
        CHECK(g.ContainsEdge(edgeId2));
        CHECK(g.ContainsEdge(nodeId1, nodeId2));
        CHECK(g.ContainsEdge(nodeId2, nodeId1));

        CHECK(node1.id == nodeId1);
        CHECK(node2.id == nodeId2);
        CHECK(edge.id != edge2.id);
        CHECK(edge.id == edgeId);
        CHECK(edge2.id == edgeId2);

        CHECK(node1.pos != node2.pos);
        CHECK(node1.pos == k_node1_pos);
        CHECK(node2.pos == k_node2_pos);

        CHECK(CheckDirectedEdgeMatch(edge, nodeId1, nodeId2));
        CHECK(!CheckDirectedEdgeMatch(edge, nodeId2, nodeId1));

        CHECK(CheckDirectedEdgeMatch(edge2, nodeId2, nodeId1));
        CHECK(!CheckDirectedEdgeMatch(edge2, nodeId1, nodeId2));

        CHECK(g.NodeHasEdges(nodeId1));
        CHECK(g.NodeHasEdges(nodeId2));
        CHECK(g.GetEdges(nodeId1).size() == 2);
        CHECK(g.GetEdges(nodeId2).size() == 2);

        CHECK(EdgeVectorContainsEdge(g.GetEdges(nodeId1), nodeId1, nodeId2));
        CHECK(EdgeVectorContainsEdge(g.GetEdges(nodeId1), nodeId2, nodeId1));
        CHECK(EdgeVectorContainsEdge(g.GetEdges(nodeId2), nodeId1, nodeId2));
        CHECK(EdgeVectorContainsEdge(g.GetEdges(nodeId2), nodeId2, nodeId1));

        CHECK(EdgeVectorContainsEdgeId(g.GetEdges(nodeId1), edgeId));
        CHECK(EdgeVectorContainsEdgeId(g.GetEdges(nodeId1), edgeId2));
        CHECK(EdgeVectorContainsEdgeId(g.GetEdges(nodeId2), edgeId));
        CHECK(EdgeVectorContainsEdgeId(g.GetEdges(nodeId2), edgeId2));
    }

    SECTION("inspection after removing the edge")
    {
        g.RemoveEdge(edgeId);

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(!g.HasEdges());

        CHECK(g.GetNodeCount() == 2);
        CHECK(g.GetEdgeCount() == 0);

        CHECK(g.GetNodes().size() == 2);
        CHECK(g.GetEdges().size() == 0);

        CHECK(g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(!g.ContainsEdge(edgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(!g.NodeHasEdges(nodeId1));
        CHECK(!g.NodeHasEdges(nodeId2));
        CHECK(g.GetEdges(nodeId1).size() == 0);
        CHECK(g.GetEdges(nodeId2).size() == 0);
    }

    SECTION("inspection after clear edges")
    {
        g.ClearEdges();

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(!g.HasEdges());

        CHECK(g.GetNodeCount() == 2);
        CHECK(g.GetEdgeCount() == 0);

        CHECK(g.GetNodes().size() == 2);
        CHECK(g.GetEdges().size() == 0);

        CHECK(g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(!g.ContainsEdge(edgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(!g.NodeHasEdges(nodeId1));
        CHECK(!g.NodeHasEdges(nodeId2));
        CHECK(g.GetEdges(nodeId1).size() == 0);
        CHECK(g.GetEdges(nodeId2).size() == 0);
    }

    SECTION("inspection after clear all")
    {
        g.Clear();

        CHECK(g.IsEmpty());
        CHECK(!g.HasNodes());
        CHECK(!g.HasEdges());

        CHECK(g.GetNodeCount() == 0);
        CHECK(g.GetEdgeCount() == 0);

        CHECK(g.GetNodes().size() == 0);
        CHECK(g.GetEdges().size() == 0);

        CHECK(!g.ContainsNode(nodeId1));
        CHECK(!g.ContainsNode(nodeId2));

        CHECK(!g.ContainsEdge(edgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));
    }

    SECTION("inspection after removing node 1")
    {
        g.RemoveNode(nodeId1);

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(!g.HasEdges());

        CHECK(g.GetNodeCount() == 1);
        CHECK(g.GetEdgeCount() == 0);

        CHECK(g.GetNodes().size() == 1);
        CHECK(g.GetEdges().size() == 0);

        CHECK(g.GetNodes()[0].id == nodeId2);
        CHECK(g.GetNodes()[0].pos == k_node2_pos);

        CHECK(!g.ContainsEdge(edgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(!g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(!g.NodeHasEdges(nodeId2));
        CHECK(g.GetEdges(nodeId2).size() == 0);
    }

    SECTION("inspection after removing node 2")
    {
        g.RemoveNode(nodeId2);

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(!g.HasEdges());

        CHECK(g.GetNodeCount() == 1);
        CHECK(g.GetEdgeCount() == 0);

        CHECK(g.GetNodes().size() == 1);
        CHECK(g.GetEdges().size() == 0);

        CHECK(g.GetNodes()[0].id == nodeId1);
        CHECK(g.GetNodes()[0].pos == k_node1_pos);

        CHECK(!g.ContainsEdge(edgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(g.ContainsNode(nodeId1));
        CHECK(!g.ContainsNode(nodeId2));

        CHECK(!g.NodeHasEdges(nodeId1));
        CHECK(g.GetEdges(nodeId1).size() == 0);
    }

    SECTION("inspection after removing both nodes")
    {
        g.RemoveNode(nodeId1);
        g.RemoveNode(nodeId2);

        CHECK(g.IsEmpty());
        CHECK(!g.HasNodes());
        CHECK(!g.HasEdges());

        CHECK(g.GetNodeCount() == 0);
        CHECK(g.GetEdgeCount() == 0);

        CHECK(g.GetNodes().size() == 0);
        CHECK(g.GetEdges().size() == 0);

        CHECK(!g.ContainsEdge(edgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(!g.ContainsNode(nodeId1));
        CHECK(!g.ContainsNode(nodeId2));
    }

    SECTION("inspection after removing the edge and adding an equivalent")
    {
        g.RemoveEdge(edgeId);
        auto newEdgeId = g.AddEdge(nodeId1, nodeId2).id;
        auto newEdge = g.GetEdge(newEdgeId);

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(g.HasEdges());

        CHECK(g.GetNodeCount() == 2);
        CHECK(g.GetEdgeCount() == 1);

        CHECK(g.GetNodes().size() == 2);
        CHECK(g.GetEdges().size() == 1);

        CHECK(g.ContainsEdge(newEdgeId));
        CHECK(g.ContainsEdge(nodeId1, nodeId2));
        CHECK(!g.ContainsEdge(nodeId2, nodeId1));

        CHECK(g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(g.NodeHasEdges(nodeId1));
        CHECK(g.NodeHasEdges(nodeId2));

        CHECK(g.GetEdges(nodeId1).size() == 1);
        CHECK(g.GetEdges(nodeId2).size() == 1);
        CHECK(CheckDirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId1, nodeId2));
        CHECK(CheckDirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId1, nodeId2));
        CHECK(!CheckDirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId2, nodeId1));
        CHECK(!CheckDirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId2, nodeId1));
        CHECK(g.GetEdges(nodeId1)[0].id == newEdgeId);
        CHECK(g.GetEdges(nodeId2)[0].id == newEdgeId);

        CHECK(CheckDirectedEdgeMatch(newEdge, nodeId1, nodeId2));
        CHECK(!CheckDirectedEdgeMatch(newEdge, nodeId2, nodeId1));
        CHECK(newEdge.id == newEdgeId);
        CHECK(newEdge.id == g.GetEdge(newEdgeId).id);
    }

    SECTION("inspection after removing the edge and adding the opposite")
    {
        g.RemoveEdge(edgeId);
        auto newEdgeId = g.AddEdge(nodeId2, nodeId1).id;
        auto newEdge = g.GetEdge(newEdgeId);

        CHECK(!g.IsEmpty());
        CHECK(g.HasNodes());
        CHECK(g.HasEdges());

        CHECK(g.GetNodeCount() == 2);
        CHECK(g.GetEdgeCount() == 1);

        CHECK(g.GetNodes().size() == 2);
        CHECK(g.GetEdges().size() == 1);

        CHECK(g.ContainsEdge(newEdgeId));
        CHECK(!g.ContainsEdge(nodeId1, nodeId2));
        CHECK(g.ContainsEdge(nodeId2, nodeId1));

        CHECK(g.ContainsNode(nodeId1));
        CHECK(g.ContainsNode(nodeId2));

        CHECK(g.NodeHasEdges(nodeId1));
        CHECK(g.NodeHasEdges(nodeId2));

        CHECK(g.GetEdges(nodeId1).size() == 1);
        CHECK(g.GetEdges(nodeId2).size() == 1);
        CHECK(CheckDirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId2, nodeId1));
        CHECK(CheckDirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId2, nodeId1));
        CHECK(!CheckDirectedEdgeMatch(g.GetEdges(nodeId1)[0], nodeId1, nodeId2));
        CHECK(!CheckDirectedEdgeMatch(g.GetEdges(nodeId2)[0], nodeId1, nodeId2));
        CHECK(g.GetEdges(nodeId1)[0].id == newEdgeId);
        CHECK(g.GetEdges(nodeId2)[0].id == newEdgeId);

        CHECK(CheckDirectedEdgeMatch(newEdge, nodeId2, nodeId1));
        CHECK(!CheckDirectedEdgeMatch(newEdge, nodeId1, nodeId2));
        CHECK(newEdge.id == newEdgeId);
        CHECK(newEdge.id == g.GetEdge(newEdgeId).id);
    }
}
