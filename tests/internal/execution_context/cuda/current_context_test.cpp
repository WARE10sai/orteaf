#include "orteaf/internal/execution_context/cuda/current_context.h"

#if ORTEAF_ENABLE_CUDA

#include "orteaf/internal/execution/cuda/api/cuda_execution_api.h"
#include <gtest/gtest.h>

namespace cuda_context = ::orteaf::internal::execution_context::cuda;
namespace cuda_api = ::orteaf::internal::execution::cuda::api;
namespace cuda_exec = ::orteaf::internal::execution::cuda;

class CudaCurrentContextTest : public ::testing::Test {
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

TEST_F(CudaCurrentContextTest, CurrentContextProvidesDefaultResources) {
  const auto &ctx = cuda_context::currentContext();
  EXPECT_TRUE(ctx.device);
  EXPECT_TRUE(ctx.context);
  EXPECT_TRUE(ctx.stream);

  auto device = cuda_context::currentDevice();
  EXPECT_TRUE(device);
  EXPECT_EQ(ctx.device.payloadHandle(), device.payloadHandle());
  EXPECT_EQ(device.payloadHandle(), cuda_exec::CudaDeviceHandle{0});
}

TEST_F(CudaCurrentContextTest, CurrentCudaContextReturnsContext) {
  auto context = cuda_context::currentCudaContext();
  EXPECT_TRUE(context);
}

TEST_F(CudaCurrentContextTest, CurrentStreamReturnsStream) {
  auto stream = cuda_context::currentStream();
  EXPECT_TRUE(stream);
}

TEST_F(CudaCurrentContextTest, SetCurrentContextOverridesState) {
  cuda_context::Context ctx{cuda_exec::CudaDeviceHandle{0}};

  cuda_context::setCurrentContext(std::move(ctx));

  const auto &current_ctx = cuda_context::currentContext();
  EXPECT_TRUE(current_ctx.device);
  EXPECT_TRUE(current_ctx.context);
  EXPECT_TRUE(current_ctx.stream);
  EXPECT_EQ(current_ctx.device.payloadHandle(), cuda_exec::CudaDeviceHandle{0});
}

TEST_F(CudaCurrentContextTest, SetCurrentOverridesState) {
  cuda_context::Context ctx{cuda_exec::CudaDeviceHandle{0}};

  cuda_context::CurrentContext current{};
  current.current = std::move(ctx);
  cuda_context::setCurrent(std::move(current));

  const auto &current_ctx = cuda_context::currentContext();
  EXPECT_TRUE(current_ctx.device);
  EXPECT_TRUE(current_ctx.context);
  EXPECT_TRUE(current_ctx.stream);
  EXPECT_EQ(current_ctx.device.payloadHandle(), cuda_exec::CudaDeviceHandle{0});
}

TEST_F(CudaCurrentContextTest, ResetReacquiresDefaultResources) {
  auto first = cuda_context::currentDevice();
  EXPECT_TRUE(first);

  cuda_context::reset();

  auto second = cuda_context::currentDevice();
  EXPECT_TRUE(second);
  EXPECT_EQ(second.payloadHandle(), cuda_exec::CudaDeviceHandle{0});

  auto context = cuda_context::currentCudaContext();
  EXPECT_TRUE(context);

  auto stream = cuda_context::currentStream();
  EXPECT_TRUE(stream);
}

TEST_F(CudaCurrentContextTest, ContextConstructorAcquiresResources) {
  cuda_context::Context ctx{cuda_exec::CudaDeviceHandle{0}};
  
  EXPECT_TRUE(ctx.device);
  EXPECT_TRUE(ctx.context);
  EXPECT_TRUE(ctx.stream);
  EXPECT_EQ(ctx.device.payloadHandle(), cuda_exec::CudaDeviceHandle{0});
}

#endif // ORTEAF_ENABLE_CUDA
