#pragma once

namespace Mirael
{

/// <summary>
/// Captures a timing metric in nanoseconds, by folding a single measurement into a bucket, to enable tracking running
/// average, min and max.  The fold operation can also reset the bucket to the given value.
/// 
/// Intended for use with the BucketCycle class for reliably transferring the metric between two threads running at differnet
/// framerates, without obscuring the extent of spikes and dips.
/// </summary>
struct FrameMetricsBucket {
    uint64_t count = 0, totalNs = 0, minNs = 0, maxNs = 0;
    void fold(uint64_t frameNs, bool reset) noexcept
    {
        if (reset) {
            count   = 1;
            totalNs = minNs = maxNs = frameNs;
        } else {
            count++;
            totalNs += frameNs;
            minNs = std::min(minNs, frameNs);
            maxNs = std::max(maxNs, frameNs);
        }
    }
    uint64_t average() const noexcept {return count ? totalNs / count : 0;}
};

} // namespace Mirael