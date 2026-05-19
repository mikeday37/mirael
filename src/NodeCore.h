#pragma once

#include <span>
#include <unordered_map>

#include "data.h"
#include "ValueBuffer.h"

namespace Mirael
{

class Runner;

class NodeCore
{
public:
    virtual ~NodeCore() = default;

protected:
    struct RunContext {
        NodeId nodeId;
        std::unordered_map<PinId, std::span<const ValueBuffer *>> inputs; // input PinId -> linked output pin value buffers
        std::unordered_map<PinId, ValueBuffer *> outputs;                 // output PinId -> output value buffer for that pin
    };

    virtual void onFrame(const RunContext &context) = 0;

    friend Runner;
};

} // namespace Mirael