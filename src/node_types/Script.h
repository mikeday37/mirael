#pragma once

#include "Mailbox.h"
#include "Node.h"

namespace Mirael::NodeTypes
{

class Script : public Node
{
public:
    static const char *typeName() { return "script"; }

protected:
    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onOrderPins(std::vector<PinId> &pinOrder) override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

    void onShowProperties() override;

    using ScriptVersion = int;

    enum class ScriptCompilationMode { None, Live, Explicit };

    enum class ScriptStatus { Empty, Good, CompileError, RuntimeError };

    struct Config {
        std::vector<PinId> inPins, outPins;
        std::string script;
        ScriptVersion scriptVersion;
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

    class Core : public NodeCore
    {
    public:
        Core(Config &&initialConfig, std::shared_ptr<Channel> channel) : config_(std::move(initialConfig)), channel_(channel) {}

    protected:
        void onFrame(const RunContext &context) override;

    private:
        Config config_{};
        std::shared_ptr<Channel> channel_;
        bool enabled_      = true;
        bool autoDisabled_ = false;

        std::string receivedScript_, runningScript_;
        ScriptVersion receivedScriptVersion_ = 0, runningScriptVersion_ = 0;
    };

    virtual std::unique_ptr<NodeCore> createCore() { return std::make_unique<Core>(buildConfig(), channel_); }

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
    ScriptVersion scriptVersion_ = 0, latestPostedScriptVersion_ = 0;
    ScriptCompilationMode compileMode_ = ScriptCompilationMode::Live;
    bool enabled_                      = true;

    // pin information
    std::vector<PinId> inputPinIds_;
    std::vector<PinId> outputPinIds_;

    // Core feedback
    CoreStatus coreStatus_{};
    bool autoDisabled_ = false;

    Config buildConfig()
    {
        return Config{.inPins = inputPinIds_, .outPins = outputPinIds_, .script = script_, .scriptVersion = scriptVersion_};
    }

    void postConfig()
    {
        channel_->pendingConfig.postNew(std::make_unique<Config>(buildConfig()));
        latestPostedScriptVersion_ = scriptVersion_;
    }

    void establishPins(PinDirection dir, const std::string &csv, std::vector<std::string> &labels, std::vector<PinId> &pinIds);
};

} // namespace Mirael::NodeTypes