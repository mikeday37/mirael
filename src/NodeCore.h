#pragma once

#include <span>
#include <unordered_map>

#include "lua.hpp"

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
        lua_State *L = nullptr;

        const ValueBuffer *getFirstInput(PinId inputPinId) const
        {
            auto it = inputs.find(inputPinId);
            if (it == inputs.end() || it->second.empty())
                return nullptr;
            else
                return it->second.front();
        }

        ValueBuffer *getOutput(PinId outputPinId) const
        {
            auto it = outputs.find(outputPinId);
            if (it == outputs.end())
                return nullptr;
            else
                return it->second;
        }
    };

    virtual void onExecutionPlanUpdated(const RunContext &context) {}
    virtual void onFrame(const RunContext &context) = 0;

    friend Runner;
};

} // namespace Mirael