#include "orteaf/internal/execution_context/mps/context.h"

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/execution/mps/api/mps_execution_api.h"

namespace orteaf::internal::execution_context::mps {

Context::Context(::orteaf::internal::execution::mps::MpsDeviceHandle device) {
  namespace mps_api = ::orteaf::internal::execution::mps::api;

  this->device = mps_api::MpsExecutionApi::acquireDevice(device);
  if (auto *resource = this->device.operator->()) {
    this->command_queue = resource->commandQueueManager().acquire();
  }
}

Context::Context(
    ::orteaf::internal::execution::mps::MpsDeviceHandle device,
    ::orteaf::internal::execution::mps::MpsCommandQueueHandle command_queue) {
  namespace mps_api = ::orteaf::internal::execution::mps::api;

  this->device = mps_api::MpsExecutionApi::acquireDevice(device);
  if (auto *resource = this->device.operator->()) {
    this->command_queue =
        resource->commandQueueManager().acquire(command_queue);
  }
}

} // namespace orteaf::internal::execution_context::mps

#endif // ORTEAF_ENABLE_MPS
