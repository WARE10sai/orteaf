#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

#include <orteaf/internal/runtime/base/lease/category.h>
#include <orteaf/internal/runtime/base/lease/slot.h>

namespace orteaf::internal::runtime::base {

/// @brief Shared control block - shared ownership with reference counting
/// @details Multiple leases can share this resource. Uses atomic reference
/// count for thread-safe sharing.
/// is_alive_ is automatically managed: true when count > 0.
template <typename SlotT>
  requires SlotConcept<SlotT>
class SharedControlBlock {
public:
  using Category = lease_category::Shared;
  using Slot = SlotT;
  using Payload = typename SlotT::Payload;

  SharedControlBlock() = default;
  SharedControlBlock(const SharedControlBlock &) = delete;
  SharedControlBlock &operator=(const SharedControlBlock &) = delete;

  SharedControlBlock(SharedControlBlock &&other) noexcept
      : is_alive_(other.is_alive_), slot_(std::move(other.slot_)) {
    strong_count_.store(other.strong_count_.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
  }

  SharedControlBlock &operator=(SharedControlBlock &&other) noexcept {
    if (this != &other) {
      is_alive_ = other.is_alive_;
      strong_count_.store(other.strong_count_.load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
      slot_ = std::move(other.slot_);
    }
    return *this;
  }

  // =========================================================================
  // Lifecycle API
  // =========================================================================

  /// @brief Acquire a shared reference, marks as alive
  /// @return always true for shared resources
  bool acquire() noexcept {
    strong_count_.fetch_add(1, std::memory_order_relaxed);
    is_alive_ = true;
    return true;
  }

  /// @brief Release a shared reference
  /// @return true if this was the last reference (count goes 1->0)
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

  /// @brief Get current reference count
  std::uint32_t count() const noexcept {
    return strong_count_.load(std::memory_order_acquire);
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

private:
  bool is_alive_{false};
  std::atomic<std::uint32_t> strong_count_{0};
  SlotT slot_{};
};

} // namespace orteaf::internal::runtime::base
