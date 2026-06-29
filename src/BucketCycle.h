#pragma once

#include <array>
#include <atomic>

namespace Mirael
{

/// <summary>
/// Implements a lock-free SPSC bucket cycle that allows one producer to fold metrics into buckets that are gauranteed to be
/// read in order by one consumer on another thread, without missing any folded information.  Producer and consumer can run
/// at any rate relative to each other, and the consumer can tell when it actually advances to the next bucket.  Producer
/// is expected to clear the bucket when isNew is true.  Consumer MUST call releaseReadBucket() when done reading.
/// </summary>
/// <typeparam name="T">The type of each element in the buffer.  Must be default constructable.</typeparam>
template <typename T> class BucketCycle
{
public:
    // producer method
    struct FetchResult {
        T &bucket;
        bool isNew;
    };
    [[nodiscard]] FetchResult fetchFoldBucket() noexcept
    {
        state_t newState, old = state_.load(std::memory_order_relaxed);
        do {
            state_t readIndex     = (old & ReadMask);
            state_t foldIndex     = (old & FoldMask) >> FoldShift;
            state_t nextFoldIndex = (foldIndex + 1) % IndexCap;

            if (nextFoldIndex == readIndex)
                return {buckets_[foldIndex], false};

            newState = readIndex | (nextFoldIndex << FoldShift);
        } while (!state_.compare_exchange_weak(old, newState, std::memory_order_acq_rel, std::memory_order_relaxed));
        return {buckets_[(newState & FoldMask) >> FoldShift], true};
    }

    // consumer methods
    const T &getReadBucket() const noexcept { return buckets_[readIndex_]; }
    bool releaseReadBucket() noexcept // returns true if the bucket advanced, meaning next read would be a new bucket
    {
        state_t newState, old = state_.load(std::memory_order_relaxed);
        do {
            state_t readIndex     = (old & ReadMask);
            state_t foldIndex     = (old & FoldMask) >> FoldShift;
            state_t nextReadIndex = (readIndex + 1) % IndexCap;

            if (nextReadIndex == foldIndex)
                return false;

            newState = nextReadIndex | (foldIndex << FoldShift);
        } while (!state_.compare_exchange_weak(old, newState, std::memory_order_acq_rel, std::memory_order_relaxed));
        readIndex_ = newState & ReadMask;
        return true;
    }

private:
    static constexpr size_t align_sz = 64; // std::hardware_destructive_interference_size; // but we're avoiding include <new>

    alignas(align_sz) std::array<T, 3> buckets_;

    /*
     * state is bit packed:
     *   bits 0-1: read index, used by consumer
     *   bits 4-5: fold index, used by producer
     */
    using state_t = uint_fast8_t;

    static constexpr state_t ReadMask = 0x03;
    static constexpr state_t FoldMask = 0x30, FoldShift = 4;

    static constexpr state_t InitialReadIndex = 0, InitialFoldIndex = 1;

    static constexpr state_t IndexCap = 3; // indices are strictly in the range 0 <= i < cap

    alignas(align_sz) std::atomic<state_t> state_ = InitialReadIndex | (InitialFoldIndex << FoldShift);
    alignas(align_sz) int readIndex_              = InitialReadIndex;
};

} // namespace Mirael