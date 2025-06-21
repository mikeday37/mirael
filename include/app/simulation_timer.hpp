#pragma once

#include <chrono>

class SimulationTimer
{
public:
    using clock = std::chrono::steady_clock;
    using timePoint = clock::time_point;
    using seconds = std::chrono::duration<float>;

    SimulationTimer() { Reset(); }

    void Reset()
    {
        lastTick_ = clock::now();
        elapsed_ = seconds{0};
        paused_ = false;
    }

    void Pause()
    {
        if (paused_) {
            return;
        }

        elapsed_ += clock::now() - lastTick_;
        paused_ = true;
    }

    void Resume()
    {
        if (!paused_) {
            return;
        }

        lastTick_ = clock::now();
        paused_ = false;
    }

    inline bool IsPaused() const { return paused_; }

    std::pair<float, float> Tick()
    { // returns {worldTime, deltaTime}
        auto now = clock::now();
        auto delta = paused_ ? seconds{0} : now - lastTick_;

        if (!paused_) {
            elapsed_ += delta;
            lastTick_ = now;
        }

        return {elapsed_.count(), delta.count()};
    }

private:
    timePoint lastTick_;
    seconds elapsed_{0};
    bool paused_ = false;
};
