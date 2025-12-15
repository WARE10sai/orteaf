#pragma once

#include <atomic>
#include <utility>

#include <orteaf/internal/runtime/base/lease/category.h>
#include <orteaf/internal/runtime/base/lease/slot.h>

namespace orteaf::internal::runtime::base {

/// @brief Unique control block - single ownership with in_use flag
/// @details Only one lease can hold this resource at a time.
/// Uses atomic CAS for thread-safe acquisition.
/// is_alive_ is automatically managed: true after acquire(), false after
/// release().
template <typename SlotT>
  requires SlotConcept<SlotT>
class UniqueControlBlock {
public:
  using Category = lease_category::Unique;
  using Slot = SlotT;
  using Payload = typename SlotT::Payload;

  UniqueControlBlock() = default;
  UniqueControlBlock(const UniqueControlBlock &) = delete;
  UniqueControlBlock &operator=(const UniqueControlBlock &) = delete;

  UniqueControlBlock(UniqueControlBlock &&other) noexcept
      : is_alive_(other.is_alive_), slot_(std::move(other.slot_)) {
    in_use_.store(other.in_use_.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
  }

  UniqueControlBlock &operator=(UniqueControlBlock &&other) noexcept {
    if (this != &other) {
      is_alive_ = other.is_alive_;
      in_use_.store(other.in_use_.load(std::memory_order_relaxed),
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

  /// @brief Release ownership, marks as not alive
  /// @return true if was in use and now released, false if wasn't in use
  bool release() noexcept {
    bool expected = true;
    if (in_use_.compare_exchange_strong(expected, false,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
      is_alive_ = false;
      if constexpr (SlotT::has_generation) {
        slot_.incrementGeneration();
      }
      return true;
    }
    return false;
  }

  /// @brief Check if resource is currently acquired/alive
  bool isAlive() const noexcept { return is_alive_; }

  // =========================================================================
  // Payload Access
  // =========================================================================

  /// @brief Access the payload
  Payload &payload() noexcept { return slot_.get(); }
  const Payload &payload() const noexcept { return slot_.get(); }

  // =========================================================================
  // Generation (delegated to Slot)
  // =========================================================================

  /// @brief Get current generation (0 if not supported)
  auto generation() const noexcept { return slot_.generation(); }

private:
  bool is_alive_{false};
  std::atomic<bool> in_use_{false};
  SlotT slot_{};
};

} // namespace orteaf::internal::runtime::base
