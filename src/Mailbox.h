#pragma once

#include <atomic>
#include <memory>

namespace Mirael
{

/// <summary>
/// Implements a lossy, lock-free, one-way, single-value, single-producer, single-consumer cross-thread communication channel.
/// New values overwrites (and frees) any previous value that hasn't yet been consumed.
/// </summary>
/// <typeparam name="T">The type of object to send across the channel.</typeparam>
template <typename T> class Mailbox
{
public:
    Mailbox() = default;

    // forbid copy/move
    Mailbox(const Mailbox &)            = delete;
    Mailbox &operator=(const Mailbox &) = delete;

    ~Mailbox()
    {
        T *taken = pending_.exchange(nullptr, std::memory_order_relaxed);
        std::unique_ptr<T> cleanup{taken};
    }

    /// <summary>
    /// Post a new value, freeing the prior if it hasn't been accepted by the consumer.
    /// </summary>
    /// <param name="newValue">The new value to post.</param>
    void postNew(std::unique_ptr<T> newValue)
    {
        T *taken = pending_.exchange(newValue.release(), std::memory_order_release);
        std::unique_ptr<T> cleanup{taken};
    }

    /// <summary>
    /// Accept and return the latest value if it exists, otherwise return nullptr.
    /// </summary>
    /// <returns>The latest posted value that wasn't previously accepted, or nullptr if there is no new value.</returns>
    std::unique_ptr<T> tryAcceptLatest()
    {
        T *taken = pending_.exchange(nullptr, std::memory_order_acquire);
        return std::unique_ptr<T>{taken};
    }

private:
    std::atomic<T *> pending_{nullptr};
};

} // namespace Mirael