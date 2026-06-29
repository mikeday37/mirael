#include "pch.h"

#include <cmath>
#include <memory>

#include "Runner.h"

namespace Mirael
{

Runner::Runner() { scriptEnv_.emplace(runContext_); }

Runner::~Runner()
{
    stop();

    for (auto &[pinId, buf] : outputPinBuffers_)
        buf->clear();

    scriptEnv_.reset();

    // the above is required before the lua state autodestructs
}

void Runner::onNewUIFrame()
{
    // TODO: impl - will be needed when runRate_.rateMode == UIRate
}

void Runner::mainLoop(std::stop_token st)
{
    updatePlan();

    while (!st.stop_requested()) {
        // start a frame and remember when we started it
        const auto frameStart          = frameClock_t::now();
        frameCoreTotalExecutionTimeNs_ = 0;

        executeFrame(st);

        const auto t1                = frameClock_t::now();
        std::chrono::nanoseconds dur = t1 - frameStart;

        // enter next frame wait loop
        do {
            const auto t2 = frameClock_t::now();

            // early out if stop requested
            if (st.stop_requested())
                return;

            // always update plan and run rate after frame and during each wakeup during the wait to next frame
            updatePlan();
            updateRunRate();

            const auto t3 = frameClock_t::now();
            dur += t3 - t2;
        } while (!waitForNextFrame(frameStart));

        foldFrameMetrics(frameCoreTotalExecutionTimeNs_, dur.count() - frameCoreTotalExecutionTimeNs_);
    }
}

bool Runner::waitForNextFrame(frameClock_t::time_point frameStart)
{
    std::optional<float> fps;

    auto waitForeverOrUntilWokenUp = [this]() {
        std::unique_lock lock(frameWaitMutex_);
        frameWaitCV_.wait(lock, [this]() { return frameWaitWakeUp_; });
        frameWaitWakeUp_ = false;
    };

    switch (runRate_.rateMode) {
    case RunRateMode::Unlimited:
        return true; // no delay, immediately run next frame

    case RunRateMode::Disabled: {
        waitForeverOrUntilWokenUp();
        return false; // delay forever (unless or until run rate setting changes)
    }

    default:
        assert(false); // unknown rate mode
        fps = 60.0f;   // we'll handle it like UI Rate in the unlikely case this happens in release
        break;

    case RunRateMode::UIRate:
        fps = 60.0f; // TODO: a future update will synchronize this more closely with the actual UI frame present timing
        break;

    case RunRateMode::SetRate:
        fps = runRate_.desiredFramesPerSecond;
        break;
    }

    assert(fps); // frame rate should always be set by this point

    // if the framerate is degenerate (negative, too small, or not finite) wait forever or until woken (to allow setting change)
    if (!std::isfinite(*fps) || *fps <= 1e-8f) {
        waitForeverOrUntilWokenUp();
        return false;
    }

    // otherwise, we do a proper framerate wait
    auto waitpoint = frameStart + std::chrono::duration_cast<frameClock_t::duration>(std::chrono::duration<float>(1.0f / *fps));
    {
        std::unique_lock lock(frameWaitMutex_);
        frameWaitCV_.wait_until(lock, waitpoint, [this]() { return frameWaitWakeUp_; });
        frameWaitWakeUp_ = false;
    }

    // only signal we're ready for the next frame if we actually passed the waitpoint
    return frameClock_t::now() >= waitpoint;
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
}

void Runner::executeFrame(std::stop_token st)
{
    if (!currentPlan_)
        return;

    for (auto nodeId : currentPlan_->nodeExecutionOrder) {
        runContext_.nodeId = nodeId;
        auto it            = cores_.find(nodeId);
        if (it != cores_.end()) {
            scriptEnv_->setCurrentNode(it->first);

            const auto t1 = frameClock_t::now();
            it->second.core->onFrame(runContext_);
            std::chrono::nanoseconds dur = frameClock_t::now() - t1;

            uint64_t durNs = dur.count();
            frameCoreTotalExecutionTimeNs_ += durNs;
            auto fetchResult = it->second.internalChannel->frameMetrics.fetchFoldBucket();
            fetchResult.bucket.fold(durNs, fetchResult.isNew);
        }
        if (st.stop_requested())
            return;
    }
}

void Runner::applyDelta(ResourceDelta &delta)
{
    for (auto deletedCoreNodeId : delta.deletedCores) {
        scriptEnv_->forgetNode(deletedCoreNodeId);
        cores_.erase(deletedCoreNodeId);
    }

    for (auto deletedOutputPinId : delta.deletedOutputs)
        outputPinBuffers_.erase(deletedOutputPinId);

    if (delta.luaEnvInitScript) {
        clearOutputBuffers();
        scriptEnv_->resetWithInitScript(*delta.luaEnvInitScript);
        initScriptResult_.postNew(std::make_unique<std::string>(scriptEnv_->getInitScriptResult()));
        raiseLuaStateReset();
    }

    for (auto addedOutputPinId : delta.addedOutputs) {
        auto [it, inserted] = outputPinBuffers_.try_emplace(addedOutputPinId, std::make_unique<ValueBuffer>(scriptEnv_->L));
        assert(inserted);
    }

    for (auto &[addedCoreNodeId, coreInfo] : delta.addedCores) {
        auto [it, inserted] = cores_.try_emplace(addedCoreNodeId, std::move(coreInfo));
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

void Runner::clearOutputBuffers()
{
    for (auto &[outputPinId, valueBuffer] : outputPinBuffers_)
        valueBuffer->clear();
}

void Runner::raiseLuaStateReset()
{
    for (auto &[outputPinId, valueBuffer] : outputPinBuffers_)
        valueBuffer->onNewLuaState(scriptEnv_->L); // doing this here is another consequence of TD1 (see TechDebt.md)

    for (auto &[nodeId, coreInfo] : cores_)
        coreInfo.core->onLuaStateReset();
}

} // namespace Mirael