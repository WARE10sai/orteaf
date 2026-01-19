#include "orteaf/internal/execution_context/cpu/context.h"

#include "orteaf/internal/execution/cpu/api/cpu_execution_api.h"

namespace orteaf::internal::execution_context::cpu {

Context::Context(::orteaf::internal::execution::cpu::CpuDeviceHandle device) {
  namespace cpu_api = ::orteaf::internal::execution::cpu::api;

  this->device = cpu_api::CpuExecutionApi::acquireDevice(device);
}

} // namespace orteaf::internal::execution_context::cpu
