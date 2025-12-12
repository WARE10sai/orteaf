#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "orteaf/internal/base/heap_vector.h"
#include "orteaf/internal/diagnostics/error/error.h"

namespace orteaf::internal::runtime::base {

/**
 * @brief Base class for runtime managers providing common lifecycle and
 * configuration logic.
 *
 * @tparam Derived The derived manager class (CRTP).
 * @tparam Traits Traits class defining DeviceType, OpsType, StateType, and
 * Name.
 */
template <typename Derived, typename Traits> class BaseManager {
public:
  using Ops = typename Traits::OpsType;
  using State = typename Traits::StateType;

  BaseManager() = default;
  BaseManager(const BaseManager &) = delete;
  BaseManager &operator=(const BaseManager &) = delete;
  BaseManager(BaseManager &&) = default;
  BaseManager &operator=(BaseManager &&) = default;
  ~BaseManager() = default;

  void setGrowthChunkSize(std::size_t chunk) {
    if (chunk == 0) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
          "Growth chunk size must be > 0");
    }
    growth_chunk_size_ = chunk;
  }

  std::size_t growthChunkSize() const noexcept { return growth_chunk_size_; }

  bool isInitialized() const noexcept { return initialized_; }

  std::size_t capacity() const noexcept { return states_.size(); }

#if ORTEAF_ENABLE_TEST
  std::size_t growthChunkSizeForTest() const noexcept {
    return growth_chunk_size_;
  }

  bool isInitializedForTest() const noexcept { return initialized_; }

  Ops *opsForTest() const noexcept { return ops_; }

  std::size_t freeListSizeForTest() const noexcept { return free_list_.size(); }

  const State &stateForTest(std::size_t index) const {
    if (index >= states_.size()) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
          "State index out of range");
    }
    return states_[index];
  }

  State &stateForTest(std::size_t index) {
    return const_cast<State &>(
        static_cast<const BaseManager *>(this)->stateForTest(index));
  }
#endif

protected:
  void ensureInitialized() const {
    if (!initialized_) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
          std::string(Traits::Name) + " has not been initialized");
    }
  }

  std::size_t allocateSlot() {
    if (free_list_.empty()) {
      growPool(growth_chunk_size_);
    }
    std::size_t index = free_list_.back();
    free_list_.resize(free_list_.size() - 1);
    return index;
  }

  void growPool(std::size_t additional) {
    std::size_t current_size = states_.size();
    states_.resize(current_size + additional);
    for (std::size_t i = 0; i < additional; ++i) {
      free_list_.pushBack(current_size + additional - 1 - i);
    }
  }

  std::size_t growth_chunk_size_{1};
  bool initialized_{false};
  Ops *ops_{nullptr};
  ::orteaf::internal::base::HeapVector<State> states_{};
  ::orteaf::internal::base::HeapVector<std::size_t> free_list_{};
};

} // namespace orteaf::internal::runtime::base
