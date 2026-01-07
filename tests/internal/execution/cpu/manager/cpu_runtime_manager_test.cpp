#include "orteaf/internal/architecture/architecture.h"
#include "orteaf/internal/architecture/cpu_detect.h"
#include "orteaf/internal/execution/cpu/manager/cpu_runtime_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace base = orteaf::internal::base;
namespace cpu_rt = orteaf::internal::execution::cpu::manager;
namespace cpu_platform = orteaf::internal::execution::cpu::platform;
namespace architecture = orteaf::internal::architecture;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

/**
 * @brief Mock implementation of CpuSlowOps for testing.
 */
class CpuSlowOpsMock : public cpu_platform::CpuSlowOps {
public:
  MOCK_METHOD(int, getDeviceCount, (), (override));
  MOCK_METHOD(architecture::Architecture, detectArchitecture,
              (base::DeviceHandle device_id), (override));
  MOCK_METHOD(void *, allocBuffer, (std::size_t size, std::size_t alignment),
              (override));
  MOCK_METHOD(void, deallocBuffer, (void *ptr, std::size_t size), (override));
};

} // namespace

class CpuRuntimeManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    manager_ = std::make_unique<cpu_rt::CpuRuntimeManager>();
  }

  void TearDown() override {
    manager_->shutdown();
    manager_.reset();
  }

  std::unique_ptr<cpu_rt::CpuRuntimeManager> manager_;
};

TEST_F(CpuRuntimeManagerTest, InitializeWithDefaultOps) {
  EXPECT_FALSE(manager_->isInitialized());

  manager_->initialize();

  EXPECT_TRUE(manager_->isInitialized());
  EXPECT_NE(manager_->slowOps(), nullptr);
  EXPECT_TRUE(manager_->deviceManager().isConfiguredForTest());
}

TEST_F(CpuRuntimeManagerTest, ShutdownClearsState) {
  manager_->initialize();
  EXPECT_TRUE(manager_->isInitialized());

  manager_->shutdown();

  EXPECT_FALSE(manager_->isInitialized());
}

TEST_F(CpuRuntimeManagerTest, DeviceManagerReturnsCorrectArch) {
  manager_->initialize();

  auto &device_manager = manager_->deviceManager();
  auto lease = device_manager.acquire(base::DeviceHandle{0});
  EXPECT_TRUE(lease);

  // Access arch through lease
  auto arch = lease.payloadPtr()->arch;
  EXPECT_EQ(arch, architecture::detectCpuArchitecture());
}

TEST_F(CpuRuntimeManagerTest, InitializeWithCustomOps) {
  auto mock_ops = std::make_unique<NiceMock<CpuSlowOpsMock>>();
  auto *mock_ptr = mock_ops.get();

  ON_CALL(*mock_ops, getDeviceCount()).WillByDefault(Return(1));
  ON_CALL(*mock_ops, detectArchitecture(base::DeviceHandle{0}))
      .WillByDefault(Return(architecture::Architecture::CpuZen4));

  manager_->initialize(std::move(mock_ops));

  EXPECT_TRUE(manager_->isInitialized());
  EXPECT_EQ(manager_->slowOps(), mock_ptr);
}

TEST_F(CpuRuntimeManagerTest, DoubleInitializeUsesExistingOps) {
  manager_->initialize();
  auto *first_ops = manager_->slowOps();

  manager_->initialize(); // Should reuse existing ops

  EXPECT_EQ(manager_->slowOps(), first_ops);
}

TEST_F(CpuRuntimeManagerTest, ReinitializeAfterShutdown) {
  manager_->initialize();
  manager_->shutdown();

  manager_->initialize();

  EXPECT_TRUE(manager_->isInitialized());
  EXPECT_TRUE(manager_->deviceManager().isConfiguredForTest());
}

TEST_F(CpuRuntimeManagerTest, DeviceManagerIsAlive) {
  manager_->initialize();

  auto &device_manager = manager_->deviceManager();
  EXPECT_TRUE(device_manager.isAliveForTest(base::DeviceHandle{0}));
}
