#include "orteaf/user/execution_context/cuda_context_guard.h"

#if ORTEAF_ENABLE_CUDA

#include "orteaf/internal/execution/cuda/api/cuda_execution_api.h"

#include <utility>

namespace orteaf::user::execution_context {
namespace {

namespace cuda_context = ::orteaf::internal::execution_context::cuda;
namespace cuda_api = ::orteaf::internal::execution::cuda::api;

cuda_context::Context makeContext(
    ::orteaf::internal::execution::cuda::CudaDeviceHandle device) {
  cuda_context::Context context{};
  context.device = cuda_api::CudaExecutionApi::acquireDevice(device);
  if (auto *device_resource = context.device.operator->()) {
    context.context = device_resource->context_manager.acquirePrimary();
    if (auto *context_resource = context.context.operator->()) {
      context.stream = context_resource->stream_manager.acquire();
    }
  }
  return context;
}

cuda_context::Context makeContext(
    ::orteaf::internal::execution::cuda::CudaDeviceHandle device,
    ::orteaf::internal::execution::cuda::CudaStreamHandle stream) {
  cuda_context::Context context{};
  context.device = cuda_api::CudaExecutionApi::acquireDevice(device);
  if (auto *device_resource = context.device.operator->()) {
    context.context = device_resource->context_manager.acquirePrimary();
    if (auto *context_resource = context.context.operator->()) {
      context.stream = context_resource->stream_manager.acquire(stream);
    }
  }
  return context;
}

} // namespace

CudaExecutionContextGuard::CudaExecutionContextGuard() {
  activate(
      makeContext(::orteaf::internal::execution::cuda::CudaDeviceHandle{0}));
}

CudaExecutionContextGuard::CudaExecutionContextGuard(
    ::orteaf::internal::execution::cuda::CudaDeviceHandle device) {
  activate(makeContext(device));
}

CudaExecutionContextGuard::CudaExecutionContextGuard(
    ::orteaf::internal::execution::cuda::CudaDeviceHandle device,
    ::orteaf::internal::execution::cuda::CudaStreamHandle stream) {
  activate(makeContext(device, stream));
}

CudaExecutionContextGuard::CudaExecutionContextGuard(
    CudaExecutionContextGuard &&other) noexcept
    : previous_(std::move(other.previous_)), active_(other.active_) {
  other.active_ = false;
}

CudaExecutionContextGuard &CudaExecutionContextGuard::operator=(
    CudaExecutionContextGuard &&other) noexcept {
  if (this != &other) {
    release();
    previous_ = std::move(other.previous_);
    active_ = other.active_;
    other.active_ = false;
  }
  return *this;
}

CudaExecutionContextGuard::~CudaExecutionContextGuard() { release(); }

void CudaExecutionContextGuard::activate(
    ::orteaf::internal::execution_context::cuda::Context context) {
  previous_.current = cuda_context::currentContext();
  cuda_context::setCurrentContext(std::move(context));
  active_ = true;
}

void CudaExecutionContextGuard::release() noexcept {
  if (!active_) {
    return;
  }
  cuda_context::setCurrent(std::move(previous_));
  active_ = false;
}

} // namespace orteaf::user::execution_context

#endif // ORTEAF_ENABLE_CUDA
