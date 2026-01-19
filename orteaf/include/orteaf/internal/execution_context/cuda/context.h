#pragma once

#if ORTEAF_ENABLE_CUDA

#include "orteaf/internal/execution/cuda/manager/cuda_context_manager.h"
#include "orteaf/internal/execution/cuda/manager/cuda_device_manager.h"
#include "orteaf/internal/execution/cuda/manager/cuda_stream_manager.h"

namespace orteaf::internal::execution_context::cuda {

class Context {
public:
  using DeviceLease =
      ::orteaf::internal::execution::cuda::manager::CudaDeviceManager::DeviceLease;
  using ContextLease = ::orteaf::internal::execution::cuda::manager::
      CudaContextManager::ContextLease;
  using StreamLease =
      ::orteaf::internal::execution::cuda::manager::CudaStreamManager::StreamLease;

  DeviceLease device{};
  ContextLease context{};
  StreamLease stream{};
};

} // namespace orteaf::internal::execution_context::cuda

#endif // ORTEAF_ENABLE_CUDA
