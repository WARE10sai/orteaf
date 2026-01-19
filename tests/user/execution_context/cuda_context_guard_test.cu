#include "orteaf/user/execution_context/cuda_context_guard.h"

#if ORTEAF_ENABLE_CUDA

#include "orteaf/internal/execution/cuda/api/cuda_execution_api.h"
#include "orteaf/internal/execution_context/cuda/current_context.h"
#include <gtest/gtest.h>

namespace user_ctx = ::orteaf::user::execution_context;
namespace cuda_api = ::orteaf::internal::execution::cuda::api;
namespace cuda_exec = ::orteaf::internal::execution::cuda;
namespace cuda_context = ::orteaf::internal::execution_context::cuda;

class CudaExecutionContextGuardTest : public ::testing::Test {
protected:
  void SetUp() override {
    cuda_api::CudaExecutionApi::ExecutionManager::Config config{};
    cuda_api::CudaExecutionApi::configure(config);
    cuda_context::reset();
  }

  void TearDown() override {
    cuda_context::reset();
    cuda_api::CudaExecutionApi::shutdown();
  }
};

TEST_F(CudaExecutionContextGuardTest, GuardRestoresPreviousContext) {
  const auto baseline_device = cuda_context::currentDevice().payloadHandle();
  const auto baseline_context = cuda_context::currentCudaContext().payloadHandle();
  const auto baseline_stream = cuda_context::currentStream().payloadHandle();

  {
    user_ctx::CudaExecutionContextGuard guard;
    auto active_device = cuda_context::currentDevice().payloadHandle();
    EXPECT_EQ(active_device, cuda_exec::CudaDeviceHandle{0});
    EXPECT_TRUE(cuda_context::currentCudaContext());
    EXPECT_TRUE(cuda_context::currentStream());
  }

  const auto restored_device = cuda_context::currentDevice().payloadHandle();
  const auto restored_context = cuda_context::currentCudaContext().payloadHandle();
  const auto restored_stream = cuda_context::currentStream().payloadHandle();
  
  EXPECT_EQ(restored_device, baseline_device);
  EXPECT_EQ(restored_context, baseline_context);
  EXPECT_EQ(restored_stream, baseline_stream);
}

TEST_F(CudaExecutionContextGuardTest, GuardWithExplicitDevice) {
  user_ctx::CudaExecutionContextGuard guard(cuda_exec::CudaDeviceHandle{0});
  
  auto active_device = cuda_context::currentDevice().payloadHandle();
  EXPECT_EQ(active_device, cuda_exec::CudaDeviceHandle{0});
  EXPECT_TRUE(cuda_context::currentCudaContext());
  EXPECT_TRUE(cuda_context::currentStream());
}

TEST_F(CudaExecutionContextGuardTest, GuardWithExplicitDeviceAndStream) {
  // First acquire a stream to get its handle
  cuda_context::Context ctx{cuda_exec::CudaDeviceHandle{0}};
  auto stream_handle = ctx.stream.payloadHandle();
  
  user_ctx::CudaExecutionContextGuard guard(cuda_exec::CudaDeviceHandle{0},
                                            stream_handle);
  
  auto active_device = cuda_context::currentDevice().payloadHandle();
  auto active_stream = cuda_context::currentStream().payloadHandle();
  
  EXPECT_EQ(active_device, cuda_exec::CudaDeviceHandle{0});
  EXPECT_EQ(active_stream, stream_handle);
}

TEST_F(CudaExecutionContextGuardTest, GuardMoveTransfersOwnership) {
  const auto baseline_device = cuda_context::currentDevice().payloadHandle();
  const auto baseline_context = cuda_context::currentCudaContext().payloadHandle();
  const auto baseline_stream = cuda_context::currentStream().payloadHandle();

  {
    user_ctx::CudaExecutionContextGuard guard;
    user_ctx::CudaExecutionContextGuard moved(std::move(guard));
    (void)moved;
  }

  const auto restored_device = cuda_context::currentDevice().payloadHandle();
  const auto restored_context = cuda_context::currentCudaContext().payloadHandle();
  const auto restored_stream = cuda_context::currentStream().payloadHandle();
  
  EXPECT_EQ(restored_device, baseline_device);
  EXPECT_EQ(restored_context, baseline_context);
  EXPECT_EQ(restored_stream, baseline_stream);
}

TEST_F(CudaExecutionContextGuardTest, GuardMoveAssignmentTransfersOwnership) {
  const auto baseline_device = cuda_context::currentDevice().payloadHandle();

  {
    user_ctx::CudaExecutionContextGuard guard1;
    user_ctx::CudaExecutionContextGuard guard2;
    guard2 = std::move(guard1);
  }

  const auto restored_device = cuda_context::currentDevice().payloadHandle();
  EXPECT_EQ(restored_device, baseline_device);
}

#endif // ORTEAF_ENABLE_CUDA
