#include "app/graph.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Compare Undirected vs Directed Graph Behavior")
{
    SECTION("undirected")
    {
        UndirectedGraph<char, int> u;
        auto a = u.AddNode('a');
        auto b = u.AddNode('b');
        CHECK(b != a);
        auto e1 = u.AddEdge(a, b, 7);
        CHECK(e1.added);
        auto e2 = u.AddEdge(b, a, 11); // this is considered to already exist (with different data)
        CHECK(!e2.added);
        CHECK(e1.id == e2.id);
        CHECK(u.EdgeData(e2.id) == 7);
        CHECK(u.GetEdgeCount() == 1);
        CHECK(u.GetEdge(a, b).id == u.GetEdge(b, a).id);
        CHECK(u.GetEdge(a, b).id == e1.id);
        CHECK(u.GetEdge(b, a).id == e1.id);
    }

    SECTION("directed")
    {
        DirectedGraph<char, int> d;
        auto a = d.AddNode('a');
        auto b = d.AddNode('b');
        CHECK(b != a);
        auto e1 = d.AddEdge(a, b, 7);
        CHECK(e1.added);
        auto e2 = d.AddEdge(b, a, 11); // this is new, as direction matters with a directed graph
        CHECK(e2.added);
        CHECK(e1.id != e2.id);
        CHECK(d.EdgeData(e2.id) == 11);
        CHECK(d.GetEdgeCount() == 2);
        CHECK(d.GetEdge(a, b).id != d.GetEdge(b, a).id);
        CHECK(d.GetEdge(a, b).id == e1.id);
        CHECK(d.GetEdge(b, a).id == e2.id);
    }
}