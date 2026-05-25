#pragma once

#include "Mailbox.h"
#include "Node.h"

namespace Mirael::NodeTypes
{

class Script : public Node
{
public:
    static const char *typeName() { return "script"; }

    struct DebugInfo {                    // used for debug purposes
        std::string graphNameWhenCreated; // will not be kept up-to-date with Graph name changes
        std::string graphUid;
        GraphId graphId;
        NodeId nodeId;
    };

    using ScriptVersion = uint64_t;
    enum class ScriptCompilationMode { None, Live, Explicit };
    enum class ScriptStatus { Empty, Good, CompileError, RuntimeError };

    struct Config {
        std::vector<PinId> inPins, outPins;
        std::string scriptNameWhenPosted = ""; // used for debug purposes - only current as of the last script post
        std::string script = "";
        ScriptVersion scriptVersion = 0;
    };

    struct CoreStatus {
        ScriptVersion receivedScriptVersion, runningScriptVersion;
        ScriptStatus scriptStatus;
        std::string errorScript;
        std::string errorText;
    };

    struct Channel {
        Mailbox<Config> pendingConfig;          // ui -> core
        Mailbox<CoreStatus> pendingCoreStatus;  // core -> ui
        std::atomic<bool> enabled      = true;  // ui -> core
        std::atomic<bool> autoDisabled = false; // core -> ui
    };

protected:
    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onOrderPins(std::vector<PinId> &pinOrder) override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

    void onShowProperties() override;

    virtual std::unique_ptr<NodeCore> createCore();

private:
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();

    // UI configuration
    std::string scriptName_;
    std::string inputsCsv_  = "in1,in2";
    std::string outputsCsv_ = "out";
    std::vector<std::string> inputLabels_;
    std::vector<std::string> outputLabels_;
    bool inlineEditor_ = true;
    std::string script_;
    ScriptVersion scriptVersion_ = 1, latestPostedScriptVersion_ = 0;
    ScriptCompilationMode compileMode_ = ScriptCompilationMode::Live;
    bool enabled_                      = true;

    // pin information
    std::vector<PinId> inputPinIds_;
    std::vector<PinId> outputPinIds_;

    // Core feedback
    CoreStatus coreStatus_{};
    bool autoDisabled_ = false;

    DebugInfo buildDebugInfo();

    Config buildConfig()
    {
        return Config{.inPins               = inputPinIds_,
                      .outPins              = outputPinIds_,
                      .scriptNameWhenPosted = scriptName_,
                      .script               = script_,
                      .scriptVersion        = scriptVersion_};
    }

    void postConfig()
    {
        channel_->pendingConfig.postNew(std::make_unique<Config>(buildConfig()));
        latestPostedScriptVersion_ = scriptVersion_;
    }

    void putEnabled() { channel_->enabled.store(enabled_, std::memory_order_relaxed); }

    void updateCoreStatus()
    {
        if (auto taken = channel_->pendingCoreStatus.tryAcceptLatest())
            coreStatus_ = std::move(*taken);

        // this inclusion of both channels is intentional
        // this function updates the entire core status view - there's no reason to separate them on the UI side
        autoDisabled_ = channel_->autoDisabled.load(std::memory_order_relaxed);
    }

    void establishPins(PinDirection dir, const std::string &csv, std::vector<std::string> &labels, std::vector<PinId> &pinIds);
};

} // namespace Mirael::NodeTypes