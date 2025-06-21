#pragma once

#include <chrono>

class FrameLimiter
{
public:
    FrameLimiter();
    FrameLimiter(std::chrono::microseconds minFramePeriod);

    void StartFrame();
    void EndFrame();

private:
    std::chrono::microseconds minFramePeriod_;
    std::chrono::steady_clock::time_point startTime_;
};
