#pragma once

/**
 * @file cuda_context_guard.h
 * @brief RAII guard for switching the CUDA execution context.
 */

#if ORTEAF_ENABLE_CUDA

#include "orteaf/internal/execution/cuda/cuda_handles.h"
#include "orteaf/internal/execution_context/cuda/current_context.h"

namespace orteaf::user::execution_context {

/**
 * @brief RAII guard that sets the CUDA execution context for its lifetime.
 *
 * Captures the current context on construction and restores it on destruction.
 * The current context is global (not thread-local).
 *
 * @par Usage (default device + primary context + new stream)
 * @code
 * #include <orteaf/user/execution_context/cuda_context_guard.h>
 *
 * using ::orteaf::user::execution_context::CudaExecutionContextGuard;
 *
 * void run_on_cuda() {
 *   CudaExecutionContextGuard guard; // uses CudaDeviceHandle{0}
 *   // CUDA work here
 * }
 * @endcode
 *
 * @par Usage (explicit device + primary context + new stream)
 * @code
 * #include <orteaf/internal/execution/cuda/cuda_handles.h>
 * #include <orteaf/user/execution_context/cuda_context_guard.h>
 *
 * CudaExecutionContextGuard guard(
 *     ::orteaf::internal::execution::cuda::CudaDeviceHandle{0});
 * @endcode
 *
 * @par Usage (explicit device + explicit stream)
 * @code
 * #include <orteaf/internal/execution/cuda/cuda_handles.h>
 * #include <orteaf/user/execution_context/cuda_context_guard.h>
 *
 * CudaExecutionContextGuard guard(
 *     ::orteaf::internal::execution::cuda::CudaDeviceHandle{0},
 *     ::orteaf::internal::execution::cuda::CudaStreamHandle{1});
 * @endcode
 *
 * @note The CUDA execution manager must be configured before creating the guard.
 */
class CudaExecutionContextGuard {
public:
  /// @brief Use the default CUDA device (handle 0), primary context, and a new stream.
  CudaExecutionContextGuard();
  /// @brief Use the specified CUDA device, primary context, and a new stream.
  explicit CudaExecutionContextGuard(
      ::orteaf::internal::execution::cuda::CudaDeviceHandle device);
  /// @brief Use the specified CUDA device and stream handles (primary context).
  CudaExecutionContextGuard(
      ::orteaf::internal::execution::cuda::CudaDeviceHandle device,
      ::orteaf::internal::execution::cuda::CudaStreamHandle stream);

  CudaExecutionContextGuard(const CudaExecutionContextGuard &) = delete;
  CudaExecutionContextGuard &
  operator=(const CudaExecutionContextGuard &) = delete;

  CudaExecutionContextGuard(CudaExecutionContextGuard &&other) noexcept;
  CudaExecutionContextGuard &
  operator=(CudaExecutionContextGuard &&other) noexcept;

  ~CudaExecutionContextGuard();

private:
  void activate(::orteaf::internal::execution_context::cuda::Context context);
  void release() noexcept;

  ::orteaf::internal::execution_context::cuda::CurrentContext previous_{};
  bool active_{false};
};

} // namespace orteaf::user::execution_context

#endif // ORTEAF_ENABLE_CUDA
