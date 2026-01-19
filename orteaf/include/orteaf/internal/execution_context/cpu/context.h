#pragma once

#include "orteaf/internal/execution/cpu/cpu_handles.h"
#include "orteaf/internal/execution/cpu/manager/cpu_device_manager.h"

namespace orteaf::internal::execution_context::cpu {

class Context {
public:
  using DeviceLease =
      ::orteaf::internal::execution::cpu::manager::CpuDeviceManager::DeviceLease;

  /// @brief Create an empty context with no resources.
  Context() = default;

  /// @brief Create a context for the specified CPU device.
  /// @param device The device handle to create the context for.
  explicit Context(::orteaf::internal::execution::cpu::CpuDeviceHandle device);

  DeviceLease device{};
};

} // namespace orteaf::internal::execution_context::cpu
