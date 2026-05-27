#include "pch.h"

#include "Runner.h"

namespace Mirael
{

Runner::Runner()
{
    establishLuaState();
}

void Runner::adjustRunRate(RunRateSetting newSetting)
{
    // TODO: impl
}

void Runner::onNewUIFrame()
{
    // TODO: impl - will be needed when runRate_.rateMode == UIRate
}

void Runner::mainLoop(std::stop_token st)
{
    updatePlan();

    while (!st.stop_requested()) {
        executeFrame(st);
        if (st.stop_requested())
            return;
        updatePlan();
        if (runRate_.rateMode != RunRateMode::Unlimited) {
            delayFrame(st);
            updatePlan();
        }
        // why update the plan twice in the loop body?
        // if we only update before the frame delay, the lag before using the latest plan will often be greater than necessary.
        // if we only update after the delay, then the time taken to update the plan may unnecessarily extend the frame delay if
        // it could have been handled before the delay.
    }
}

void Runner::updatePlan()
{
    if (!try_acceptLatestPlan())
        return;
    assert(currentPlan_); // we'll always have a plan from this point forward

    if (pendingFutureDelta_ && pendingFutureDelta_->version <= currentPlan_->version) {
        applyDelta(*pendingFutureDelta_);
        pendingFutureDelta_.reset();
    }

    // TODO: directly clarify why we're doing it this way, or link to doc

    if (!pendingFutureDelta_) {
        std::unique_ptr<ResourceDelta> delta;
        while (try_dequeueDelta(delta)) {
            if (delta->version > currentPlan_->version) {
                pendingFutureDelta_ = std::move(delta);
                break;
            } else {
                applyDelta(*delta);
            }
        }
    }

    prepareRunContext();

    for (auto &[nodeId, core] : cores_)
        core->onExecutionPlanUpdated(runContext_);
}

void Runner::executeFrame(std::stop_token st)
{
    if (!currentPlan_)
        return;

    runContext_.L = L;

    for (auto nodeId : currentPlan_->nodeExecutionOrder) {
        runContext_.nodeId = nodeId;
        auto it            = cores_.find(nodeId);
        if (it != cores_.end())
            it->second->onFrame(runContext_);
        if (st.stop_requested())
            return;
    }
}

void Runner::delayFrame(std::stop_token st)
{
    // TODO: impl - for now we're just using a hard-coded delay to avoid CPU waste
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 90));
}

void Runner::applyDelta(ResourceDelta &delta)
{
    for (auto deletedNodeId : delta.deletedCores)
        cores_.erase(deletedNodeId);

    for (auto deletedOutputPinId : delta.deletedOutputs)
        outputPinBuffers_.erase(deletedOutputPinId);

    for (auto addedOutputPinId : delta.addedOutputs) {
        auto [it, inserted] = outputPinBuffers_.try_emplace(addedOutputPinId, std::make_unique<ValueBuffer>(L));
        assert(inserted);
    }

    for (auto &[addedCoreNodeId, core] : delta.addedCores) {
        auto [it, inserted] = cores_.try_emplace(addedCoreNodeId, std::move(core));
        assert(inserted);
    }
}

void Runner::prepareRunContext()
{
    runContext_.nodeId = 0;

    // TODO: in the future we could probably do this with less memory churn, but because run contexts are only prepared after
    // topology edits, rather than every frame, this is acceptable for now

    runContext_.inputs.clear();
    runContext_.outputs.clear();
    inputsBackingVectors_.clear();

    for (auto [outputPinId, inputPinId] : currentPlan_->valueLinks) {
        auto it = inputsBackingVectors_.find(inputPinId);
        if (it == inputsBackingVectors_.end()) {
            it = inputsBackingVectors_.try_emplace(inputPinId, std::vector<const ValueBuffer *>()).first;
        }
        it->second.push_back(outputPinBuffers_.at(outputPinId).get());
    }

    for (auto &[inputPinId, backingVector] : inputsBackingVectors_)
        runContext_.inputs.try_emplace(inputPinId, std::span(backingVector));

    for (auto &[outputPinId, valueBuffer] : outputPinBuffers_)
        runContext_.outputs.try_emplace(outputPinId, valueBuffer.get());
}

void Runner::establishLuaState()
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

} // namespace Mirael