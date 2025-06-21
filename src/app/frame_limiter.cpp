#include "app_pch.hpp"

#include "app/frame_limiter.hpp"
#include <chrono>
#include <thread>

using steady_clock = std::chrono::steady_clock;
using microseconds = std::chrono::microseconds;

constexpr microseconds kDefaultMinFramePeriod{5000}; // 5ms = 1/200s

FrameLimiter::FrameLimiter() : minFramePeriod_(kDefaultMinFramePeriod) {}
FrameLimiter::FrameLimiter(microseconds minFramePeriod) : minFramePeriod_(minFramePeriod) {}

void FrameLimiter::StartFrame() { startTime_ = steady_clock::now(); }

void FrameLimiter::EndFrame()
{
    auto endTime = steady_clock::now();
    auto duration = std::chrono::duration_cast<microseconds>(endTime - startTime_);

    if (duration < minFramePeriod_) {
        std::this_thread::sleep_for(minFramePeriod_ - duration);
    }
}
