#include "orteaf/internal/execution/mps/resource/mps_kernel_metadata.h"

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/execution/mps/api/mps_execution_api.h"
#include "orteaf/internal/execution/mps/resource/mps_kernel_base.h"
#include "orteaf/internal/kernel/core/kernel_entry.h"
#include "orteaf/internal/kernel/core/kernel_metadata.h"

namespace orteaf::internal::execution::mps::resource {

void MpsKernelMetadata::rebuildKernelEntry(
    ::orteaf::internal::kernel::core::KernelEntry &entry) const {
  entry.setBase(
      ::orteaf::internal::execution::mps::api::MpsExecutionApi::acquireKernelBase(
          keys()));
}

::orteaf::internal::kernel::core::KernelMetadataLease
MpsKernelMetadata::buildMetadataLeaseFromBase(
    ::orteaf::internal::execution::mps::resource::MpsKernelBase &base) {
  ::orteaf::internal::kernel::core::KernelMetadataLease metadata;
  metadata.setLease(
      ::orteaf::internal::execution::mps::api::MpsExecutionApi::acquireKernelMetadata(
          base.keys()));
  return metadata;
}

} // namespace orteaf::internal::execution::mps::resource

#endif // ORTEAF_ENABLE_MPS
