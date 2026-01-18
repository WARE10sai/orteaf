#include "orteaf/internal/execution/cuda/manager/cuda_module_manager.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

#if ORTEAF_ENABLE_CUDA

namespace cuda_rt = orteaf::internal::execution::cuda::manager;
namespace cuda_platform = orteaf::internal::execution::cuda::platform;
namespace cuda_wrapper = orteaf::internal::execution::cuda::platform::wrapper;

namespace {

struct TestModuleHooks {
  int load_file_calls{0};
  int load_image_calls{0};
  int get_function_calls{0};
  int unload_calls{0};
  cuda_wrapper::CudaModule_t module{
      reinterpret_cast<cuda_wrapper::CudaModule_t>(0x1)};
  cuda_wrapper::CudaFunction_t function{
      reinterpret_cast<cuda_wrapper::CudaFunction_t>(0x2)};
  std::string last_kernel_name{};
};

TestModuleHooks *g_hooks = nullptr;

class TestCudaSlowOps final : public cuda_platform::CudaSlowOps {
public:
  int getDeviceCount() override { return 1; }

  cuda_wrapper::CudaDevice_t getDevice(std::uint32_t) override {
    return cuda_wrapper::CudaDevice_t{0};
  }

  cuda_wrapper::ComputeCapability getComputeCapability(
      cuda_wrapper::CudaDevice_t) override {
    return cuda_wrapper::ComputeCapability{0, 0};
  }

  std::string getDeviceName(cuda_wrapper::CudaDevice_t) override {
    return "mock-cuda";
  }

  std::string getDeviceVendor(cuda_wrapper::CudaDevice_t) override {
    return "mock";
  }

  cuda_wrapper::CudaContext_t getPrimaryContext(
      cuda_wrapper::CudaDevice_t) override {
    return context_;
  }

  cuda_wrapper::CudaContext_t createContext(
      cuda_wrapper::CudaDevice_t) override {
    return context_;
  }

  void setContext(cuda_wrapper::CudaContext_t context) override {
    last_context_ = context;
  }

  void releasePrimaryContext(cuda_wrapper::CudaDevice_t) override {}

  void releaseContext(cuda_wrapper::CudaContext_t) override {}

  cuda_wrapper::CudaStream_t createStream() override { return nullptr; }

  void destroyStream(cuda_wrapper::CudaStream_t) override {}

  cuda_wrapper::CudaEvent_t createEvent() override { return nullptr; }

  void destroyEvent(cuda_wrapper::CudaEvent_t) override {}

  void setContextForTest(cuda_wrapper::CudaContext_t context) {
    context_ = context;
  }

  cuda_wrapper::CudaContext_t lastContext() const noexcept {
    return last_context_;
  }

  cuda_wrapper::CudaModule_t loadModuleFromFile(const char *) override {
    if (g_hooks) {
      ++g_hooks->load_file_calls;
    }
    return g_hooks ? g_hooks->module : nullptr;
  }

  cuda_wrapper::CudaModule_t loadModuleFromImage(const void *) override {
    if (g_hooks) {
      ++g_hooks->load_image_calls;
    }
    return g_hooks ? g_hooks->module : nullptr;
  }

  cuda_wrapper::CudaFunction_t getFunction(cuda_wrapper::CudaModule_t,
                                           const char *name) override {
    if (g_hooks) {
      ++g_hooks->get_function_calls;
      g_hooks->last_kernel_name = name ? name : "";
    }
    return g_hooks ? g_hooks->function : nullptr;
  }

  void unloadModule(cuda_wrapper::CudaModule_t) override {
    if (g_hooks) {
      ++g_hooks->unload_calls;
    }
  }

private:
  cuda_wrapper::CudaContext_t context_{nullptr};
  cuda_wrapper::CudaContext_t last_context_{nullptr};
};

} // namespace

class CudaModuleManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    g_hooks = &hooks_;
    slow_ops_ = std::make_unique<TestCudaSlowOps>();
    manager_ = std::make_unique<cuda_rt::CudaModuleManager>();
    context_ = reinterpret_cast<cuda_wrapper::CudaContext_t>(0x1);
    slow_ops_->setContextForTest(context_);
  }

  void TearDown() override {
    manager_->shutdown();
    manager_.reset();
    slow_ops_.reset();
    g_hooks = nullptr;
  }

  void configureManager() {
    cuda_rt::CudaModuleManager::Config config{};
    manager_->configureForTest(config, context_, slow_ops_.get());
  }

  TestModuleHooks hooks_{};
  std::unique_ptr<TestCudaSlowOps> slow_ops_;
  std::unique_ptr<cuda_rt::CudaModuleManager> manager_;
  cuda_wrapper::CudaContext_t context_{nullptr};
};

TEST_F(CudaModuleManagerTest, ConfigureSucceeds) {
  configureManager();
  EXPECT_TRUE(manager_->isConfiguredForTest());
}

TEST_F(CudaModuleManagerTest, AcquireCachesByKey) {
  configureManager();

  auto lease1 = manager_->acquire(cuda_rt::ModuleKey::File("module.bin"));
  auto lease2 = manager_->acquire(cuda_rt::ModuleKey::File("module.bin"));

  EXPECT_TRUE(lease1);
  EXPECT_TRUE(lease2);
  EXPECT_EQ(hooks_.load_file_calls, 1);
}

TEST_F(CudaModuleManagerTest, GetFunctionCachesResult) {
  configureManager();

  auto lease = manager_->acquire(cuda_rt::ModuleKey::File("module.bin"));
  auto fn1 = manager_->getFunction(lease, "kernel_a");
  auto fn2 = manager_->getFunction(lease, "kernel_a");

  EXPECT_EQ(fn1, fn2);
  EXPECT_EQ(hooks_.get_function_calls, 1);
  EXPECT_EQ(hooks_.last_kernel_name, "kernel_a");
  EXPECT_EQ(slow_ops_->lastContext(), context_);
}

TEST_F(CudaModuleManagerTest, ShutdownUnloadsModule) {
  configureManager();
  auto lease = manager_->acquire(cuda_rt::ModuleKey::File("module.bin"));
  EXPECT_TRUE(lease);
  lease.release();
  manager_->shutdown();
  EXPECT_EQ(hooks_.unload_calls, 1);
}

TEST_F(CudaModuleManagerTest, InvalidKeyThrows) {
  configureManager();
  cuda_rt::ModuleKey key{};
  EXPECT_THROW(manager_->acquire(key), std::system_error);
}

TEST_F(CudaModuleManagerTest, NotConfiguredThrows) {
  EXPECT_THROW(manager_->acquire(cuda_rt::ModuleKey::File("module.bin")),
               std::system_error);
}

#endif // ORTEAF_ENABLE_CUDA
