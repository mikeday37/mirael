#pragma once

#include <span>
#include <unordered_map>

#include "lua.hpp"

#include "data.h"
#include "ValueBuffer.h"

namespace Mirael
{

class Runner;
class ScriptEnv;

class NodeCore
{
public:
    virtual ~NodeCore() = default;

protected:
    struct RunContext {
        NodeId nodeId;
        std::unordered_map<PinId, std::span<const ValueBuffer *>> inputs; // input PinId -> linked output pin value buffers
        std::unordered_map<PinId, ValueBuffer *> outputs;                 // output PinId -> output value buffer for that pin
        lua_State *L   = nullptr;
        ScriptEnv *env = nullptr;

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

    virtual void onFrame(const RunContext &context) = 0;

    virtual void onLuaStateReset() {}; // any lua refs kept by the core must be discarded (not released) when this is called

    friend Runner;
    friend ScriptEnv;
};

} // namespace Mirael