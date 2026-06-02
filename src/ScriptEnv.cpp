#include "pch.h"

#include "ScriptEnv.h"

namespace Mirael
{

ScriptEnv::ScriptEnv(NodeCore::RunContext &runContext) : runContext_(runContext)
{
    runContext.env = this;
    establishLuaState();
}

void ScriptEnv::registerPins(PinMapping pinMapping)
{
    pinMappings_[runContext_.nodeId] = pinMapping;
    setCurrentNode(runContext_.nodeId); // this is necessary for the case where a core calls registerPins() within onFrame
}

void ScriptEnv::pushEnvTable()
{
    assert(envTableRef_ != LUA_NOREF); // TODO: switching to std::optional<int> would make this assert cleaner
    lua_rawgeti(L, LUA_REGISTRYINDEX, envTableRef_);
}

void ScriptEnv::resetWithInitScript(const std::string &initScript)
{
    L_.reset();
    L            = nullptr;
    envTableRef_ = LUA_NOREF;
    establishLuaState(initScript.c_str());
}

void ScriptEnv::establishLuaState(const char *initScript)
{
    if (L_)
        return;

    assert(!L);

    L_.reset(luaL_newstate());

    if (!L_)
        throw std::runtime_error("Unable to initialize Lua.");

    L             = L_.get();
    runContext_.L = L;

    luaL_openlibs(L);

    establishRootMiraelKeywords();

    attemptInitScript(initScript); // it is valid for init scripts to modfiy globals, or make aliases to mirael keywords

    establishEnvTable(); // globals are locked here
}

void ScriptEnv::attemptInitScript(const char *initScript)
{
    if (!initScript) {
        initScriptResult_ = "No script.";
        return;
    }

    auto len = std::strlen(initScript);
    if (!len) {
        initScriptResult_ = "Empty script.";
        return;
    }

    int ret = luaL_loadbuffer(L, initScript, len, "initScript");

    if (ret != LUA_OK) {
        initScriptResult_ = std::format("Compilation Error: {}", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }

    ret = lua_pcall(L, 0, 0, 0);

    if (ret != LUA_OK) {
        initScriptResult_ = std::format("Runtime Error: {}", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }

    initScriptResult_ = "Success."; // NOTE: this text-only feedback is a consequence of TD1 (see TechDebt.md)
}

void ScriptEnv::establishRootMiraelKeywords()
{
    lua_pushvalue(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "input");
    pushNewUserData(l_inputIndex, nullptr, l_inputCall);
    lua_rawset(L, -3);

    lua_pushstring(L, "output");
    pushNewUserData(l_outputIndex, l_outputNewIndex, l_outputCall);
    lua_rawset(L, -3);

    lua_pop(L, 1);
}

void ScriptEnv::establishEnvTable()
{
    assert(envTableRef_ == LUA_NOREF);

    lua_newtable(L); // the env table itself
    lua_newtable(L); // env's metatable

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -2, "__index"); // env reads passthrough to globals

    lua_pushcfunction(L, l_forbidGlobalNewIndex);
    lua_setfield(L, -2, "__newindex"); // forbid writing to env

    lua_setmetatable(L, -2);

    envTableRef_ = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_pushstring(L, "_G");
    lua_rawgeti(L, LUA_REGISTRYINDEX, envTableRef_);
    lua_rawset(L, -3); // _G now points to env, making it effictively a read-only version of its former self
    lua_pop(L, 1);

    assert(!lua_gettop(L));
}

void ScriptEnv::pushNewUserData(lua_CFunction indexFn, lua_CFunction newIndexFn, lua_CFunction callFn)
{
    lua_newuserdata(L, 0);
    lua_newtable(L); // the userdata's metatable

    if (indexFn) {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, indexFn, 1);
        lua_setfield(L, -2, "__index");
    }

    if (newIndexFn) {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, newIndexFn, 1);
        lua_setfield(L, -2, "__newindex");
    }

    if (callFn) {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, callFn, 1);
        lua_setfield(L, -2, "__call");
    }

    lua_setmetatable(L, -2);
}

int ScriptEnv::l_forbidGlobalNewIndex(lua_State *L)
{
    const char *key = luaL_checkstring(L, 2);
    return luaL_error(L, "attempt to write at global index \'%s\'", key);
}

int ScriptEnv::l_inputIndex(lua_State *L)
{
    auto *self = static_cast<ScriptEnv *>(lua_touserdata(L, lua_upvalueindex(1)));
    int n      = static_cast<int>(lua_tointeger(L, 2));

    PinId pinId;
    if (!tryGetPinId(self->currentInPins_, n, pinId)) {
        lua_pushnil(L);
        return 1;
    }

    const ValueBuffer *buf = self->runContext_.getFirstInput(pinId);
    if (!buf) {
        lua_pushnil(L);
        return 1;
    }

    buf->pushValueToLuaStack();
    return 1;
}

int ScriptEnv::l_inputCall(lua_State *L)
{
    auto *self = static_cast<ScriptEnv *>(lua_touserdata(L, lua_upvalueindex(1)));

    auto &pins = *self->currentInPins_;
    for (auto pinId : pins) {
        const auto *buf = self->runContext_.getFirstInput(pinId);
        if (buf)
            buf->pushValueToLuaStack();
        else
            lua_pushnil(L);
    }

    return static_cast<int>(pins.size());
}

int ScriptEnv::l_outputIndex(lua_State *L)
{
    auto *self = static_cast<ScriptEnv *>(lua_touserdata(L, lua_upvalueindex(1)));
    int n      = static_cast<int>(lua_tointeger(L, 2));

    PinId pinId;
    if (!tryGetPinId(self->currentOutPins_, n, pinId)) {
        lua_pushnil(L);
        return 1;
    }

    const ValueBuffer *buf = self->runContext_.getOutput(pinId);
    if (!buf) {
        lua_pushnil(L);
        return 1;
    }

    buf->pushValueToLuaStack();
    return 1;
}

int ScriptEnv::l_outputNewIndex(lua_State *L)
{
    auto *self = static_cast<ScriptEnv *>(lua_touserdata(L, lua_upvalueindex(1)));
    int n      = static_cast<int>(lua_tointeger(L, 2));

    PinId pinId;
    if (!tryGetPinId(self->currentOutPins_, n, pinId))
        return 0;

    ValueBuffer *buf = self->runContext_.getOutput(pinId);
    if (!buf)
        return 0;

    lua_pushvalue(L, 3);
    buf->setValueFromLuaStack();
    return 0;
}

int ScriptEnv::l_outputCall(lua_State *L)
{
    auto *self        = static_cast<ScriptEnv *>(lua_touserdata(L, lua_upvalueindex(1)));
    auto &pins        = *self->currentOutPins_;
    const int numPins = static_cast<int>(pins.size());
    const int numArgs = lua_gettop(L) - 1;

    if (numArgs > 0) {
        // set outputs to provided list
        const int excess = numArgs - numPins;

        if (excess > 0)
            lua_pop(L, excess);

        int i = std::min(numArgs, numPins);

        while (i-- > 0) {
            auto pinId = pins[i];
            auto *buf  = self->runContext_.getOutput(pinId);
            if (buf)
                buf->setValueFromLuaStack(); // pops the value from the Lua stack and sets the buffer to that value
        }

        return 0;
    } else {
        // return all outputs
        for (auto pinId : pins) {
            const auto *buf = self->runContext_.getOutput(pinId);
            if (buf)
                buf->pushValueToLuaStack();
            else
                lua_pushnil(L);
        }

        return numPins;
    }
}

bool ScriptEnv::tryGetPinId(const std::vector<PinId> *pins, int n, PinId &outPinId)
{
    if (pins && n > 0 && n <= pins->size()) {
        outPinId = (*pins)[static_cast<size_t>(n - 1)];
        return true;
    } else
        return false;
}

void ScriptEnv::setCurrentNode(NodeId nodeId)
{
    auto it = pinMappings_.find(nodeId);
    if (it != pinMappings_.end()) {
        currentInPins_  = it->second.inPins;
        currentOutPins_ = it->second.outPins;
    } else {
        currentInPins_  = nullptr;
        currentOutPins_ = nullptr;
    }
}

void ScriptEnv::forgetNode(NodeId nodeId) { pinMappings_.erase(nodeId); }

}; // namespace Mirael