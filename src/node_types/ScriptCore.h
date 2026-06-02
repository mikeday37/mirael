#pragma once

#include "lua.hpp"

#include "Mailbox.h"
#include "NodeCore.h"
#include "Script.h"

namespace Mirael::NodeTypes::Cores
{

using DebugInfo     = Script::DebugInfo;
using ScriptVersion = Script::ScriptVersion;
using ScriptStatus  = Script::ScriptStatus;
using Config        = Script::Config;
using CoreStatus    = Script::CoreStatus;
using Channel       = Script::Channel;
using ErrorMode     = Script::RuntimeErrorHandlingMode;

class ScriptCore : public NodeCore
{
public:
    ScriptCore(std::shared_ptr<Channel> channel, DebugInfo &&debugInfo) : channel_(channel), debugInfo_(std::move(debugInfo)) {}

private:
    std::shared_ptr<Channel> channel_;
    Config config_{};
    CoreStatus status_{};
    bool enabled_      = true;
    bool autoDisabled_ = false;

    std::string receivedScript_, runningScript_;

    DebugInfo debugInfo_{};

    void postStatus() { channel_->pendingCoreStatus.postNew(std::make_unique<CoreStatus>(status_)); }

    void putAutoDisabled() { channel_->autoDisabled.store(autoDisabled_, std::memory_order_relaxed); }
    bool tryAcceptLatestConfig()
    {
        if (auto taken = channel_->pendingConfig.tryAcceptLatest()) {
            config_ = std::move(*taken);
            return true;
        } else
            return false;
    }

    bool getEnabled() { return channel_->enabled.load(std::memory_order_relaxed); }
    ErrorMode getErrorMode() { return channel_->errorMode.load(std::memory_order_relaxed); }

    lua_State *L = nullptr; // handy alias for context.L
    std::string chunkName_{"(none)"};
    std::optional<int> chunkRef_{};
    bool needHandleLuaStateReset_ = false;

    void compileNewScript();
    void updatePinAccess(const RunContext &context);
    void runScript(const RunContext &context);

protected:
    void onFrame(const RunContext &context) override;
    void onLuaStateReset() override;
};

} // namespace Mirael::NodeTypes::Cores