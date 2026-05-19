#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "readerwriterqueue.h"

#include "data.h"
#include "NodeCore.h"

namespace Mirael
{

using PlanVersion = uint64_t;

struct ResourceDelta {
    PlanVersion version = 0;
    std::vector<NodeId> deletedCores;
    std::unordered_map<NodeId, std::unique_ptr<NodeCore>> addedCores;
    std::vector<PinId> deletedOutputs;
    std::vector<PinId> addedOutputs;
};

struct ExecutionPlan {
    PlanVersion version;
    std::vector<NodeId> nodeExecutionOrder;
    struct Link {
        PinId output, input;
    };
    std::vector<Link> valueLinks; // only includes links that tie an input to an output - excludes node-internal sublinks
};

enum class RunRateMode { Disabled = 0, SetRate = 1, UIRate = 2, Unlimited = 3 };

struct RunRateSetting {
    RunRateMode rateMode;
    float desiredFramesPerSecond;
};

class Runner
{
public:
    Runner() = default;

    // forbid copy, move
    Runner(const Runner &)            = delete;
    Runner &operator=(const Runner &) = delete;
    Runner(Runner &&)                 = delete;
    Runner &operator=(Runner &&)      = delete;

    // Graph API
    void run(RunRateSetting runRate)
    {
        if (thread_) {
            adjustRunRate(runRate);
            return;
        }
        runRate_ = runRate;
        thread_ = std::jthread([this](std::stop_token st) { mainLoop(st); });
    }
    void stop() { if (thread_) thread_->request_stop(); thread_.reset();}
    void adjustRunRate(RunRateSetting newSetting);
    void onNewUIFrame();
    void queueDelta(std::unique_ptr<ResourceDelta> delta) { deltaQueue_.enqueue(std::move(delta)); }
    void postPlan(std::unique_ptr<ExecutionPlan> newPlan)
    {
        ExecutionPlan *prevPlan = pendingPlan_.exchange(newPlan.release(), std::memory_order_acq_rel);
        std::unique_ptr<ExecutionPlan> cleanup{prevPlan};
    }

private:
    // Graph API communications channels/buffer
    moodycamel::ReaderWriterQueue<std::unique_ptr<ResourceDelta>> deltaQueue_{};
    std::unique_ptr<ResourceDelta> pendingFutureDelta_{}; // used to store up to 1 dequeued delta for a future plan version
    std::atomic<ExecutionPlan *> pendingPlan_{nullptr};
    std::unique_ptr<ExecutionPlan> currentPlan_{};

    // Receiving Graph Communications
    bool try_dequeueDelta(std::unique_ptr<ResourceDelta> &out) { return deltaQueue_.try_dequeue(out); }
    bool try_acceptLatestPlan()
    {
        ExecutionPlan *taken = pendingPlan_.exchange(nullptr, std::memory_order_acq_rel);
        if (!taken)
            return false;
        currentPlan_.reset(taken);
        return true;
    }

    // main operations
    void mainLoop(std::stop_token st);

    void updatePlan();
    void executeFrame(std::stop_token st);
    void delayFrame(std::stop_token st);

    void applyDelta(ResourceDelta &delta);
    void prepareRunContext();

    // our thread
    std::unordered_map<NodeId, std::unique_ptr<NodeCore>> cores_;
    std::unordered_map<PinId, std::unique_ptr<ValueBuffer>> outputPinBuffers_;
    std::unordered_map<PinId, std::vector<const ValueBuffer *>> inputsBackingVectors_;
    NodeCore::RunContext runContext_;
    RunRateSetting runRate_ = {.rateMode = RunRateMode::Disabled, .desiredFramesPerSecond = 60.0f};
    std::optional<std::jthread> thread_{};
};

} // namespace Mirael