#pragma once

#include "app/graph.hpp"
#include <string>
#include <vector>

class GraphManipulator
{
public:
    virtual ~GraphManipulator() = default;

    virtual const char *GetDisplayName() const = 0;
    virtual void OnShowControls() {}

    virtual void Manipulate(Graph &g) = 0;
};

class GenerateRandomGraphManipulator : public GraphManipulator
{
public:
    const char *GetDisplayName() const override { return "Generate Random"; }
    void OnShowControls() override;

    void Manipulate(Graph &g) override;

private:
    int nodeCount_ = 10;
    int edgeCount_ = 25;
    int maxEdgeTries_ = 15;
    float scale_ = 0.9f;
};

class GenerateGridGraphManipulator : public GraphManipulator
{
public:
    const char *GetDisplayName() const override { return "Generate Grid"; }
    void OnShowControls() override;

    void Manipulate(Graph &g) override;

private:
    int width_ = 10;
    int height_ = 10;
    float scale_ = 0.9f;
    bool includeEdges_ = true;
};

class TangleGraphManipulator : public GraphManipulator
{
public:
    const char *GetDisplayName() const override { return "Tangle"; };
    void OnShowControls() override;

    void Manipulate(Graph &g) override;

private:
    enum class Shape { Random = 0, Circle = 1 };
    inline static constexpr const char *ShapeNames[] = {"Random", "Circle"};
    Shape shape_ = Shape::Circle;
    float scale_ = 0.9f;
};

class CullGraphManipulator : public GraphManipulator
{
public:
    const char *GetDisplayName() const override { return "Cull Nodes/Edges"; }
    void OnShowControls() override;

    void Manipulate(Graph &g) override;

private:
    float nodeFraction_ = 0;
    float edgeFraction_ = 0.1f;
};

struct KnownGraphManipulators {
    GenerateRandomGraphManipulator random;
    GenerateGridGraphManipulator grid;
    TangleGraphManipulator tangle;
    CullGraphManipulator cull;

    std::vector<GraphManipulator *> GetAll()
    {
        return {
            &random,
            &grid,
            &tangle,
            &cull,
        };
    }
};
