#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

#include <orteaf/internal/runtime/base/lease/category.h>
#include <orteaf/internal/runtime/base/lease/slot.h>

namespace orteaf::internal::runtime::base {

/// @brief Weak-shared control block - shared ownership with weak reference
/// support
/// @details Like std::shared_ptr with std::weak_ptr support. Reference counted
/// with separate strong and weak counts.
/// is_alive_ is automatically managed: true when count > 0.
template <typename SlotT>
  requires SlotConcept<SlotT>
class WeakSharedControlBlock {
public:
  using Category = lease_category::WeakShared;
  using Slot = SlotT;
  using Payload = typename SlotT::Payload;

  WeakSharedControlBlock() = default;
  WeakSharedControlBlock(const WeakSharedControlBlock &) = delete;
  WeakSharedControlBlock &operator=(const WeakSharedControlBlock &) = delete;

  WeakSharedControlBlock(WeakSharedControlBlock &&other) noexcept
      : is_alive_(other.is_alive_), slot_(std::move(other.slot_)) {
    strong_count_.store(other.strong_count_.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
    weak_count_.store(other.weak_count_.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
  }

  WeakSharedControlBlock &operator=(WeakSharedControlBlock &&other) noexcept {
    if (this != &other) {
      is_alive_ = other.is_alive_;
      strong_count_.store(other.strong_count_.load(std::memory_order_relaxed),
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

  /// @brief Acquire a strong reference, marks as alive
  /// @return always true for shared resources
  bool acquire() noexcept {
    strong_count_.fetch_add(1, std::memory_order_relaxed);
    is_alive_ = true;
    return true;
  }

  /// @brief Release a strong reference
  /// @return true if this was the last strong reference
  bool release() noexcept {
    if (strong_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
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
  // Shared-specific API (SharedControlBlockConcept)
  // =========================================================================

  /// @brief Get current strong reference count
  std::uint32_t count() const noexcept {
    return strong_count_.load(std::memory_order_acquire);
  }

  // =========================================================================
  // Weak Reference API (WeakableControlBlockConcept)
  // =========================================================================

  /// @brief Acquire a weak reference
  void acquireWeak() noexcept {
    weak_count_.fetch_add(1, std::memory_order_relaxed);
  }

  /// @brief Release a weak reference
  /// @return true if this was the last reference (strong and weak both zero)
  bool releaseWeak() noexcept {
    const auto prev = weak_count_.fetch_sub(1, std::memory_order_acq_rel);
    return prev == 1 && strong_count_.load(std::memory_order_acquire) == 0;
  }

  /// @brief Try to promote weak reference to strong
  /// @return true if successfully promoted (strong count was > 0)
  bool tryPromote() noexcept {
    std::uint32_t current = strong_count_.load(std::memory_order_acquire);
    while (current > 0) {
      if (strong_count_.compare_exchange_weak(current, current + 1,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed)) {
        is_alive_ = true;
        return true;
      }
    }
    return false;
  }

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

  /// @brief Get weak reference count
  std::uint32_t weakCount() const noexcept {
    return weak_count_.load(std::memory_order_acquire);
  }

private:
  bool is_alive_{false};
  std::atomic<std::uint32_t> strong_count_{0};
  std::atomic<std::uint32_t> weak_count_{0};
  SlotT slot_{};
};

} // namespace orteaf::internal::runtime::base
