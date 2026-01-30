#pragma once

#include <utility>
#include <variant>

#if ORTEAF_ENABLE_MPS
#include "orteaf/internal/execution/mps/manager/mps_kernel_metadata_manager.h"
#endif

namespace orteaf::internal::kernel {

/**
 * @brief Type-erased kernel metadata lease.
 */
class KernelMetadataLease {
public:
#if ORTEAF_ENABLE_MPS
  using MpsKernelMetadataLease =
      ::orteaf::internal::execution::mps::manager::MpsKernelMetadataManager::
          KernelMetadataLease;
#endif

  using Variant = std::variant<
      std::monostate
#if ORTEAF_ENABLE_MPS
      ,
      MpsKernelMetadataLease
#endif
      >;

  KernelMetadataLease() = default;

  explicit KernelMetadataLease(Variant lease) noexcept
      : lease_(std::move(lease)) {}

  Variant &lease() noexcept { return lease_; }
  const Variant &lease() const noexcept { return lease_; }

  void setLease(Variant lease) noexcept { lease_ = std::move(lease); }

private:
  Variant lease_{};
};

} // namespace orteaf::internal::kernel
