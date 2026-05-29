#pragma once

#include "lua.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

#include "NodeCore.h"

namespace Mirael
{

/*
 * ScriptEnv manages the Lua Environment for Script Nodes.
 *
 * ScriptEnv is owned by the Runner and operates only on the Runner thread.
 */
class ScriptEnv final
{
public:
    explicit ScriptEnv(NodeCore::RunContext &runContext);

    // forbid copy/move
    ScriptEnv(const ScriptEnv &)            = delete;
    ScriptEnv &operator=(const ScriptEnv &) = delete;
    ScriptEnv(ScriptEnv &&)                 = delete;
    ScriptEnv &operator=(ScriptEnv &&)      = delete;

    struct PinMapping {
        const std::vector<PinId> *inPins;
        const std::vector<PinId> *outPins;
    };

    void registerPins(PinMapping pinMapping);
    void pushChunkEnv();

private:
    NodeCore::RunContext &runContext_;
    struct LuaStateDeleter {
        void operator()(lua_State *s) const { lua_close(s); }
    };
    std::unique_ptr<lua_State, LuaStateDeleter> L_{nullptr}; // lives for the life of the runner
    lua_State *L = nullptr;                                  // handy alias for L_.get();

    void establishLuaState();

    std::unordered_map<NodeId, PinMapping> pinMappings_;

    const std::vector<PinId> *currentInPins_  = nullptr;
    const std::vector<PinId> *currentOutPins_ = nullptr;

    int chunkEnvRef_ = LUA_NOREF;

    void establishRootMiraelKeywords();
    void establishEnvTable();
    void pushNewUserData(lua_CFunction indexFn, lua_CFunction newIndexFn, lua_CFunction callFn);

    static int l_forbidGlobalNewIndex(lua_State *L);
    static int l_inputIndex(lua_State *L);
    static int l_inputCall(lua_State *L);
    static int l_outputIndex(lua_State *L);
    static int l_outputNewIndex(lua_State *L);
    static int l_outputCall(lua_State *L);

    static bool tryGetPinId(const std::vector<PinId> *pins, int n, PinId &outPinId);

    void setCurrentNode(NodeId nodeId);
    void forgetNode(NodeId nodeId);

    friend Runner;
};

} // namespace Mirael