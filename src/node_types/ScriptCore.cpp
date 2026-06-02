#include "pch.h"

#include "lua.hpp"

#include "ScriptCore.h"
#include "ScriptEnv.h"

namespace Mirael::NodeTypes::Cores
{

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

void ScriptCore::updatePinAccess(const RunContext &context)
{
    assert(context.env);
    context.env->registerPins({.inPins = &config_.inPins, .outPins = &config_.outPins});
}

void ScriptCore::runScript(const RunContext &context)
{
    if (autoDisabled_ || !chunkRef_ || !getEnabled())
        return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, *chunkRef_);
    context.env->pushEnvTable();
    lua_setfenv(L, -2);
    auto ret = lua_pcall(L, 0, 0, 0);

    if (ret != LUA_OK) {
        bool willReport = status_.scriptStatus != ScriptStatus::CompileError;
        auto *errorText = lua_tostring(L, -1);
        if (willReport) {
            if (errorText)
                status_.errorText = errorText;
            else
                status_.errorText.clear();
        }
        lua_pop(L, 1);
        if (getErrorMode() == ErrorMode::AutoDisable)
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
    L = context.L;

    bool configUpdated = tryAcceptLatestConfig() && config_.scriptVersion > status_.receivedScriptVersion;
    if (configUpdated || needHandleLuaStateReset_) {
        if (needHandleLuaStateReset_ && !configUpdated) {
            config_.script = receivedScript_; // this is an awkward kludge.  see TD1 in TechDebt.md
        }
        needHandleLuaStateReset_ = false;
        compileNewScript();
        updatePinAccess(context);
    }

    assert(status_.receivedScriptVersion >
           0); // the above should always compile a script on first frame, as the UI side should post config before create

    runScript(context);
}

void ScriptCore::onLuaStateReset()
{
    L                        = nullptr;
    chunkRef_                = LUA_NOREF;
    needHandleLuaStateReset_ = true;
}

} // namespace Mirael::NodeTypes::Cores
