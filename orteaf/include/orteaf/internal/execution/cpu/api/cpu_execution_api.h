#pragma once

#include "orteaf/internal/diagnostics/error/error.h"
#include "orteaf/internal/execution/cpu/manager/cpu_execution_manager.h"
#include "orteaf/internal/execution/cpu/platform/cpu_slow_ops.h"
#include "orteaf/internal/execution/cpu/cpu_handles.h"

namespace orteaf::internal::execution::cpu::api {

class CpuExecutionApi {
public:
  using ExecutionManager =
      ::orteaf::internal::execution::cpu::manager::CpuExecutionManager;
  using DeviceHandle = ::orteaf::internal::execution::cpu::CpuDeviceHandle;
  using DeviceLease =
      ::orteaf::internal::execution::cpu::manager::CpuDeviceManager::DeviceLease;
  using SlowOps = ::orteaf::internal::execution::cpu::platform::CpuSlowOps;

  CpuExecutionApi() = delete;

  static void configure(const ExecutionManager::Config &config) {
    runtime().configure(config);
  }

  static void shutdown() { runtime().shutdown(); }

  static DeviceLease acquireDevice(DeviceHandle device) {
    Runtime &rt = runtime();
    auto device_lease = rt.deviceManager().acquire(device);
    if (!device_lease.operator->()) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
          "CPU device lease has no payload");
    }
    return device_lease;
  }

private:
  static ExecutionManager &runtime() {
    static ExecutionManager instance{};
    return instance;
  }
};

} // namespace orteaf::internal::execution::cpu::api
