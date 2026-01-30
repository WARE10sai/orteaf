#include "orteaf/internal/execution/mps/resource/mps_kernel_metadata.h"

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/execution/mps/api/mps_execution_api.h"
#include "orteaf/internal/execution/mps/resource/mps_kernel_base.h"
#include "orteaf/internal/kernel/core/kernel_metadata.h"

namespace orteaf::internal::execution::mps::resource {

void MpsKernelMetadata::rebuildKernelEntry(
    ::orteaf::internal::kernel::core::KernelEntry &entry) const {
  entry.setBase(
      ::orteaf::internal::execution::mps::api::MpsExecutionApi::acquireKernelBase(
          keys()));
  entry.setExecute(execute());
}

::orteaf::internal::kernel::core::KernelMetadataLease
MpsKernelMetadata::buildMetadataLeaseFromBase(const MpsKernelBase &base,
                                              ExecuteFunc execute) {
  ::orteaf::internal::kernel::core::KernelMetadataLease metadata;
  metadata.setLease(
      ::orteaf::internal::execution::mps::api::MpsExecutionApi::acquireKernelMetadata(
          base.keys(), execute));
  return metadata;
}

} // namespace orteaf::internal::execution::mps::resource

#endif // ORTEAF_ENABLE_MPS
