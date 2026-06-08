#pragma once

#include <array>
#include <atomic>

namespace Mirael
{

/// <summary>
/// Implements a non-blocking triple buffer that always reads the most recent committed slot
/// even if the writer is much slower than the reader.
/// </summary>
/// <typeparam name="T">The type of each element in the buffer.</typeparam>
template <typename T> class TripleBuffer
{
public:
    // producer methods
    T &getWriteSlot() noexcept { return slots_[writeIndex_]; }
    void commitWrite() noexcept
    {
        state_t newState, old = state_.load(std::memory_order_relaxed);
        do {
            // swap write/transfer, set new data
            newState = (old & ReadMask)                                          // leave read index unchanged
                       | (((old & WriteMask) >> WriteShift) << TransferShift)    // old write index -> transfer index
                       | (((old & TransferMask) >> TransferShift) << WriteShift) // old transfer index -> write index
                       | NewDataBit;                                             // set new data bit
        } while (!state_.compare_exchange_weak(old, newState, std::memory_order_release, std::memory_order_relaxed));
        writeIndex_ = (newState & WriteMask) >> WriteShift;
    }

    // consumer method
    struct FetchResult {
        const T &slot;
        bool isNew;
    };
    [[nodiscard]] FetchResult fetchLatestReadSlot() noexcept
    {
        state_t newState, old = state_.load(std::memory_order_relaxed);
        do {
            if (!(old & NewDataBit)) // if nothing new, change nothing
                return {slots_[(old & ReadMask) >> ReadShift], false};

            // otherwise, swap read/transfer, clear new data
            newState = (old & WriteMask)                                        // leave write index unchanged
                       | (((old & ReadMask) >> ReadShift) << TransferShift)     // old read index -> transfer index
                       | (((old & TransferMask) >> TransferShift) << ReadShift) // old transfer index -> read index
                ;                                                               // new data bit is left cleared
        } while (!state_.compare_exchange_weak(old, newState, std::memory_order_acquire, std::memory_order_relaxed));
        return {slots_[(newState & ReadMask) >> ReadShift], true};
    }

private:
    static constexpr size_t align_sz = 64; // std::hardware_destructive_interference_size; // but we're avoiding include <new>

    alignas(align_sz) std::array<T, 3> slots_;

    /*
     * state is bit packed:
     *   bits 0-1: read index, used by consumer
     *   bits 2-3: transfer index
     *   bits 4-5: write index, used by producer
     *      bit 6: new data available
     */
    using state_t = uint_fast8_t;

    static constexpr state_t ReadMask = 0x03, ReadShift = 0;
    static constexpr state_t TransferMask = 0x0C, TransferShift = 2;
    static constexpr state_t WriteMask = 0x30, WriteShift = 4;
    static constexpr state_t NewDataBit = 0x40;

    static constexpr int InitialReadIndex = 0, InitialTransferIndex = 1, InitialWriteIndex = 2;

    alignas(align_sz) std::atomic<state_t> state_ = InitialReadIndex | (InitialTransferIndex << TransferShift) |
                                                    (InitialWriteIndex << WriteShift);
    alignas(align_sz) int writeIndex_ = InitialWriteIndex;
};

} // namespace Mirael