#pragma once

#include <orteaf/internal/runtime/base/lease/category.h>
#include <orteaf/internal/runtime/base/lease/slot.h>

namespace orteaf::internal::runtime::base {

/// @brief Raw control block - no reference counting
/// @details Used for resources that don't need lifecycle management.
/// is_alive_ is automatically managed: true after acquire(), false after
/// release().
template <typename SlotT>
  requires SlotConcept<SlotT>
class RawControlBlock {
public:
  using Category = lease_category::Raw;
  using Slot = SlotT;
  using Payload = typename SlotT::Payload;

  RawControlBlock() = default;
  RawControlBlock(const RawControlBlock &) = default;
  RawControlBlock &operator=(const RawControlBlock &) = default;
  RawControlBlock(RawControlBlock &&) = default;
  RawControlBlock &operator=(RawControlBlock &&) = default;

  // =========================================================================
  // Lifecycle API
  // =========================================================================

  /// @brief Acquire the resource, marks as alive
  /// @return always true for raw resources
  bool acquire() noexcept {
    is_alive_ = true;
    return true;
  }

  /// @brief Release and prepare for reuse, marks as not alive
  /// @return always true for raw resources
  bool release() noexcept {
    is_alive_ = false;
    if constexpr (SlotT::has_generation) {
      slot_.incrementGeneration();
    }
    return true;
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
  SlotT slot_{};
};

} // namespace orteaf::internal::runtime::base
