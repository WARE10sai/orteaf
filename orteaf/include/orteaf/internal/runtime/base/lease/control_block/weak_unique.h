#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

#include <orteaf/internal/runtime/base/lease/category.h>
#include <orteaf/internal/runtime/base/lease/slot.h>

namespace orteaf::internal::runtime::base {

/// @brief Weak-unique control block - single ownership with weak reference
/// support
/// @details Allows weak references to observe the resource without owning it.
/// The resource is destroyed when the strong owner releases, but control block
/// persists until all weak references are gone.
/// is_alive_ is automatically managed: true after acquire(), false after
/// release().
template <typename SlotT>
  requires SlotConcept<SlotT>
class WeakUniqueControlBlock {
  // WeakUnique does not support generation tracking.
  static_assert(!SlotT::has_generation,
                "WeakUniqueControlBlock does not support generation tracking. "
                "Use RawSlot<T> instead.");

public:
  using Category = lease_category::WeakUnique;
  using Slot = SlotT;
  using Payload = typename SlotT::Payload;

  WeakUniqueControlBlock() = default;
  WeakUniqueControlBlock(const WeakUniqueControlBlock &) = delete;
  WeakUniqueControlBlock &operator=(const WeakUniqueControlBlock &) = delete;

  WeakUniqueControlBlock(WeakUniqueControlBlock &&other) noexcept
      : is_alive_(other.is_alive_), slot_(std::move(other.slot_)) {
    in_use_.store(other.in_use_.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
    weak_count_.store(other.weak_count_.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
  }

  WeakUniqueControlBlock &operator=(WeakUniqueControlBlock &&other) noexcept {
    if (this != &other) {
      is_alive_ = other.is_alive_;
      in_use_.store(other.in_use_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
      weak_count_.store(other.weak_count_.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
      slot_ = std::move(other.slot_);
    }
    return *this;
  }

  // =========================================================================
  // Lifecycle API
  // =========================================================================

  /// @brief Acquire exclusive ownership, marks as alive
  /// @return true if successfully acquired, false if already in use
  bool acquire() noexcept {
    bool expected = false;
    if (in_use_.compare_exchange_strong(expected, true,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
      is_alive_ = true;
      return true;
    }
    return false;
  }

  /// @brief Release strong ownership, marks as not alive
  /// @return true if was in use and now released, false if wasn't in use
  bool release() noexcept {
    bool expected = true;
    if (in_use_.compare_exchange_strong(expected, false,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
      is_alive_ = false;
      return true;
    }
    return false;
  }

  /// @brief Check if resource is currently acquired/alive
  bool isAlive() const noexcept { return is_alive_; }

  // =========================================================================
  // Weak Reference API (WeakableControlBlockConcept)
  // =========================================================================

  /// @brief Acquire a weak reference
  void acquireWeak() noexcept {
    weak_count_.fetch_add(1, std::memory_order_relaxed);
  }

  /// @brief Release a weak reference
  /// @return true if this was the last weak reference and resource is not in
  /// use
  bool releaseWeak() noexcept {
    const auto prev = weak_count_.fetch_sub(1, std::memory_order_acq_rel);
    return prev == 1 && !in_use_.load(std::memory_order_acquire);
  }

  /// @brief Try to promote weak reference to strong
  /// @return true if successfully promoted
  bool tryPromote() noexcept { return acquire(); }

  // =========================================================================
  // Payload Access
  // =========================================================================

  /// @brief Access the payload
  Payload &payload() noexcept { return slot_.get(); }
  const Payload &payload() const noexcept { return slot_.get(); }

  // =========================================================================
  // Additional Queries
  // =========================================================================

  /// @brief Get weak reference count
  std::uint32_t weakCount() const noexcept {
    return weak_count_.load(std::memory_order_acquire);
  }

private:
  bool is_alive_{false};
  std::atomic<bool> in_use_{false};
  std::atomic<std::uint32_t> weak_count_{0};
  SlotT slot_{};
};

} // namespace orteaf::internal::runtime::base
