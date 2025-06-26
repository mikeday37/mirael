#include "app_pch.hpp"

#include "app/applet_graph_types.hpp"
#include "app/graph_manipulators.hpp"
#include "imgui.h"
#include <random>
#include <ranges>

void GenerateRandomGraphManipulator::OnShowControls()
{
    ImGui::SliderInt("Node Count", &nodeCount_, 1, 100);
    ImGui::SliderInt("Edge Count", &edgeCount_, 0, nodeCount_ * (nodeCount_ - 1) / 2);
    ImGui::SliderInt("Max Edge Tries", &maxEdgeTries_, 0, 100);
    ImGui::SliderFloat("Scale", &scale_, 0.001f, 1.0f);
}

void GenerateRandomGraphManipulator::Manipulate(UntangleAppletGraph &g)
{

    // this method will satisfy nodeCount_ and will make a good effort on edgeCount_
    // when nearly saturated, though, it may create fewer edges than edgeCount_.
    // It's not valuable to take this further and efficiently create any random graph
    // with guaranteed edge count as this is really just for illustrative purposes.

    g.Clear();

    if (nodeCount_ < 1) {
        return;
    }

    std::vector<int> nodeIds;
    nodeIds.resize(nodeCount_);

    std::mt19937 rng(std::random_device{}());

    std::uniform_real_distribution<float> coordPicker(-1.0f * scale_, 1.0f * scale_);

    for (int index : std::views::iota(0, nodeCount_)) {
        auto nodeId = g.AddNode(coordPicker(rng), coordPicker(rng));
        assert(nodeId);
        nodeIds[index] = nodeId;
    }

    // guard some special cases
    if (nodeCount_ < 2) { // edges can't exist
        return;
    } else if (nodeCount_ == 2) { // there can be only one
        g.AddEdge(nodeIds[0], nodeIds[1]);
        return;
    }

    int maxEdgeCount = (nodeCount_ - 1) * nodeCount_ / 2;
    if (edgeCount_ >= maxEdgeCount) { // complete graph, no need for randomness and retries
        for (int indexA : std::views::iota(0, nodeCount_ - 1)) {
            for (int indexB : std::views::iota(indexA + 1, nodeCount_)) {
                g.AddEdge(nodeIds[indexA], nodeIds[indexB]);
            }
        }
        return;
    }

    // for the multiple possible edges but not all case:
    // we're guaranteeing indexA != indexB and that the distribution isn't skewed by that requirement

    std::uniform_int_distribution<int> nodePickerA(0, nodeCount_ - 1);
    std::uniform_int_distribution<int> nodePickerB(0, nodeCount_ - 2);

    auto doWithMaxTries = [](int maxTries, auto &&func) -> bool {
        for (int _ : std::views::iota(0, maxTries)) {
            unused(_);
            if (func())
                return true;
        }
        return false;
    };

    for (int _ : std::views::iota(0, edgeCount_)) {
        unused(_);
        int indexA = nodePickerA(rng);
        int indexB = nodePickerB(rng);
        if (indexB >= indexA) {
            ++indexB;
        }
        assert(indexA != indexB);
        doWithMaxTries(maxEdgeTries_,
                       [&g, &nodeIds, indexA, indexB] { return g.AddEdge(nodeIds[indexA], nodeIds[indexB]).added; });
    }
}

void GenerateGridGraphManipulator::OnShowControls()
{
    ImGui::SliderInt("Width", &width_, 1, 100);
    ImGui::SliderInt("Height", &height_, 1, 100);
    ImGui::SliderFloat("Scale", &scale_, 0.001f, 1.0f);
    ImGui::Checkbox("Include Edges", &includeEdges_);
}

void GenerateGridGraphManipulator::Manipulate(UntangleAppletGraph &g)
{
    g.Clear();

    if (width_ < 0 || height_ < 0) {
        return;
    }

    std::vector<int> nodeIds; // grid stored like screen pixels : left to right, top to bottom
    nodeIds.reserve(static_cast<size_t>(width_) * static_cast<size_t>(height_));

    auto lerp = [this](int v, int extent) -> float {
        if (extent < 2) {
            return 0;
        } else {
            return (2.0f / static_cast<float>(extent - 1) * static_cast<float>(v) - 1.0f) * scale_;
        }
    };
    for (auto y : std::views::iota(0, height_)) {
        for (auto x : std::views::iota(0, width_)) {
            nodeIds.push_back(g.AddNode(lerp(x, width_), lerp(y, height_)));
        }
    }

    if (!includeEdges_) {
        return;
    }

    auto nodeIdAt = [this, nodeIds](int x, int y) -> int {
        assert(x >= 0);
        assert(x < width_);
        assert(y >= 0);
        assert(y < height_);

        return nodeIds[y * width_ + x];
    };

    for (auto y : std::views::iota(0, height_)) {
        for (auto x : std::views::iota(0, width_)) {
            if (x > 0) {
                g.AddEdge(nodeIdAt(x, y), nodeIdAt(x - 1, y));
            }
            if (y > 0) {
                g.AddEdge(nodeIdAt(x, y), nodeIdAt(x, y - 1));
            }
        }
    }
}

void TangleGraphManipulator::OnShowControls()
{
    int shapeIndex = static_cast<int>(shape_);
    if (ImGui::Combo("Shape", &shapeIndex, ShapeNames, std::size(ShapeNames))) {
        shape_ = static_cast<Shape>(shapeIndex);
    }
    ImGui::SliderFloat("Scale", &scale_, 0.001f, 1.0f);
}

void TangleGraphManipulator::Manipulate(UntangleAppletGraph &g)
{
    std::mt19937 rng(std::random_device{}());
    switch (shape_) {
    case Shape::Random: {
        std::uniform_real_distribution<float> coordPicker(-1.0f * scale_, 1.0f * scale_);
        for (auto node : g.GetNodes()) {
            g.RepositionNode(node.id, {coordPicker(rng), coordPicker(rng)});
        }
        break;
    }

    case Shape::Circle: {
        auto nodes = g.GetNodes();
        std::vector<int> nodeIndices;
        auto size = static_cast<int>(nodes.size());
        nodeIndices.reserve(size);
        for (auto node : nodes) {
            nodeIndices.push_back(node.id);
        }
        float thetaStep = glm::two_pi<float>() / static_cast<float>(size);
        for (auto index : std::views::iota(0, size)) {
            auto nodeId = RemoveRandomElement(nodeIndices, rng);
            auto theta = static_cast<float>(index) * thetaStep - glm::half_pi<float>();
            g.RepositionNode(nodeId, {glm::cos(theta) * scale_, glm::sin(theta) * scale_});
        }
        break;
    }
    }
}

void CullGraphManipulator::OnShowControls()
{
    ImGui::SliderFloat("Node Fraction", &nodeFraction_, 0, 1);
    ImGui::SliderFloat("Edge Fraction", &edgeFraction_, 0, 1);
}

void CullGraphManipulator::Manipulate(UntangleAppletGraph &g)
{
    std::mt19937 rng(std::random_device{}());
    auto cull = [&g, &rng](bool nodes, int count, float fraction, auto &&getFunc, auto &&removeFunc) {
        using Item = std::ranges::range_value_t<std::invoke_result_t<decltype(getFunc)>>;
        static_assert(requires(Item item) { item.id; }, "Item type returned by getFunc() must have an .id field.");
        int num = static_cast<int>(std::round(static_cast<float>(count) * fraction));
        if (num <= 0) {
            return;
        } else if (num >= count) {
            if (nodes) {
                g.Clear();
            } else {
                g.ClearEdges();
            }
            return;
        }

        auto items = getFunc();
        for (auto _ : std::views::iota(0, num)) {
            unused(_);
            auto item = RemoveRandomElement(items, rng);
            removeFunc(item.id);
        }
    };
    cull(true, g.GetNodeCount(), nodeFraction_, [&g] { return g.GetNodes(); }, [&g](int id) { g.RemoveNode(id); });
    cull(false, g.GetEdgeCount(), edgeFraction_, [&g] { return g.GetEdges(); }, [&g](int id) { g.RemoveEdge(id); });
}
