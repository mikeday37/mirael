#include "pch.h"

#include "ScriptEnv.h"

namespace Mirael
{

ScriptEnv::ScriptEnv(NodeCore::RunContext &runContext) : runContext_(runContext)
{
    runContext.env = this;
    establishLuaState();
    establishEnvTable();
}

ScriptEnv::~ScriptEnv()
{
    auto unref = [this](int &ref) {
        if (ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, ref);
            ref = LUA_NOREF;
        }
    };
    unref(inputUserdataRef_);
    unref(outputUserdataRef_);
    unref(chunkEnvRef_);

    // cleanup of the lua_State is handled via RAII
    // TODO: see if we can make RAII wrappers for the above in a worthwhile way
}

void ScriptEnv::registerPins(PinMapping pinMapping)
{
    pinMappings_[runContext_.nodeId] = pinMapping;
    setCurrentNode(runContext_.nodeId); // this is necessary for the case where a core calls registerPins() within onFrame
}

void ScriptEnv::pushChunkEnv()
{
    assert(chunkEnvRef_ != LUA_NOREF); // TODO: switching to std::optional<int> would make this assert cleaner
    lua_rawgeti(L, LUA_REGISTRYINDEX, chunkEnvRef_);
}

void ScriptEnv::establishLuaState()
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
}

void ScriptEnv::establishEnvTable()
{
    assert(inputUserdataRef_ == LUA_NOREF);
    inputUserdataRef_ = createArrayAccessTable("mirael.input", l_inputIndex, nullptr);

    assert(outputUserdataRef_ == LUA_NOREF);
    outputUserdataRef_ = createArrayAccessTable("mirael.output", l_outputIndex, l_outputNewIndex);

    lua_newtable(L); // the env table itself
    lua_rawgeti(L, LUA_REGISTRYINDEX, inputUserdataRef_);
    lua_setfield(L, -2, "input");
    lua_rawgeti(L, LUA_REGISTRYINDEX, outputUserdataRef_);
    lua_setfield(L, -2, "output");

    lua_newtable(L); // env's metatable, will fallthrough to globals
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    assert(chunkEnvRef_ == LUA_NOREF);
    chunkEnvRef_ = luaL_ref(L, LUA_REGISTRYINDEX);
}

[[nodiscard]] int ScriptEnv::createArrayAccessTable(const char *name, lua_CFunction indexFn, lua_CFunction newIndexFn)
{
    lua_newuserdata(L, 0);

    luaL_newmetatable(L, name);

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

    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");

    lua_setmetatable(L, -2);

    return luaL_ref(L, LUA_REGISTRYINDEX);
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
    if (!tryGetPinId(self->currentOutPins_, n, pinId)) {
        lua_pushnil(L);
        return 1;
    }

    ValueBuffer *buf = self->runContext_.getOutput(pinId);
    if (!buf) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushvalue(L, 3);
    buf->setValueFromLuaStack();
    return 1;
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