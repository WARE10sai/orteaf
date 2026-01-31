#pragma once

#if ORTEAF_ENABLE_MPS

#include <utility>

#include "orteaf/internal/base/heap_vector.h"
#include "orteaf/internal/execution/mps/manager/mps_compute_pipeline_state_manager.h"
#include "orteaf/internal/execution/mps/manager/mps_library_manager.h"
#include "orteaf/internal/kernel/core/kernel_entry.h"
#include "orteaf/internal/execution/mps/resource/mps_kernel_base.h"

namespace orteaf::internal::kernel::core {
class KernelMetadataLease;
}

namespace orteaf::internal::execution::mps::resource {

struct MpsKernelBase;

/**
 * @brief Kernel metadata resource for MPS.
 *
 * Stores library/function keys for kernel reconstruction.
 */
struct MpsKernelMetadata {
  using LibraryKey = ::orteaf::internal::execution::mps::manager::LibraryKey;
  using FunctionKey = ::orteaf::internal::execution::mps::manager::FunctionKey;
  using Key = std::pair<LibraryKey, FunctionKey>;

  bool initialize(const ::orteaf::internal::base::HeapVector<Key> &keys) {
    reset();
    keys_.reserve(keys.size());
    for (const auto &key : keys) {
      keys_.pushBack(key);
    }
    return true;
  }

  void reset() noexcept {
    keys_.clear();
  }

  const ::orteaf::internal::base::HeapVector<Key> &keys() const noexcept {
    return keys_;
  }

  void rebuildKernelEntry(
      ::orteaf::internal::kernel::core::KernelEntry &entry) const;

  static ::orteaf::internal::kernel::core::KernelMetadataLease
  buildMetadataLeaseFromBase(::orteaf::internal::execution::mps::resource::MpsKernelBase &base);

private:
  ::orteaf::internal::base::HeapVector<Key> keys_{};
};

} // namespace orteaf::internal::execution::mps::resource

#endif // ORTEAF_ENABLE_MPS
