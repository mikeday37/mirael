#include "pch.h"

#include "lua.hpp"

#include "ScriptCore.h"

namespace Mirael::NodeTypes::Cores
{

void ScriptCore::establishLuaState()
{
    if (L_)
        return;
    assert(!L);

    L_.reset(luaL_newstate());

    if (!L_)
        throw std::runtime_error("Unable to initialize Lua.");

    L = L_.get();

    luaL_openlibs(L);
}

void ScriptCore::updatePinAccess() {}

void ScriptCore::compileNewScript()
{
    receivedScript_               = std::move(config_.script);
    status_.receivedScriptVersion = config_.scriptVersion;

    chunkName_ = std::format("{}/{}.v{}={}/{}", debugInfo_.graphId, debugInfo_.nodeId, config_.scriptVersion,
                             debugInfo_.graphNameWhenCreated, config_.scriptNameWhenPosted);

    auto ret = luaL_loadbuffer(L, receivedScript_.data(), receivedScript_.size(), chunkName_.c_str());

    if (ret != LUA_OK) {
        status_.errorText = lua_tostring(L, -1);
        lua_pop(L, 1);
        status_.errorScript  = receivedScript_;
        status_.scriptStatus = ScriptStatus::CompileError;
        postStatus();
        return;
    }

    if (chunkRef_)
        luaL_unref(L, LUA_REGISTRYINDEX, *chunkRef_);
    chunkRef_ = luaL_ref(L, LUA_REGISTRYINDEX);

    runningScript_               = receivedScript_;
    status_.runningScriptVersion = status_.receivedScriptVersion;

    autoDisabled_ = false;
    putAutoDisabled();

    status_.errorScript.clear();
    status_.scriptStatus = ScriptStatus::Good;

    postStatus();
}

void ScriptCore::runScript()
{
    if (autoDisabled_ || !chunkRef_ || !getEnabled())
        return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, *chunkRef_);
    auto ret = lua_pcall(L, 0, LUA_MULTRET, 0);

    if (ret != LUA_OK) {
        bool willReport = status_.scriptStatus != ScriptStatus::CompileError;
        auto *errorText = lua_tostring(L, -1);
        if (willReport)
            status_.errorText = errorText;
        lua_pop(L, 1);
        autoDisabled_ = true;
        putAutoDisabled();
        if (willReport) {
            status_.errorScript  = runningScript_;
            status_.scriptStatus = ScriptStatus::RuntimeError;
            postStatus();
        }
        return;
    }

    if (status_.scriptStatus == ScriptStatus::RuntimeError) {
        status_.errorScript.clear();
        status_.scriptStatus = ScriptStatus::Good;
        postStatus();
    }
}

void ScriptCore::onFrame(const RunContext &context)
{
    establishLuaState();

    auto priorReceivedScriptVersion = status_.receivedScriptVersion;
    if (tryAcceptLatestConfig()) {
        // updatePinAccess();

        if (config_.scriptVersion > priorReceivedScriptVersion)
            compileNewScript();
    }

    runScript();
}

} // namespace Mirael::NodeTypes::Cores
