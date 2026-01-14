#include <gtest/gtest.h>

#include <system_error>

#include "orteaf/internal/execution/cpu/api/cpu_execution_api.h"
#include "orteaf/internal/storage/manager/storage_manager.h"
#include "orteaf/internal/storage/storage.h"

namespace storage = orteaf::internal::storage;
namespace storage_manager = orteaf::internal::storage::manager;
namespace cpu_api = orteaf::internal::execution::cpu::api;
namespace cpu = orteaf::internal::execution::cpu;

class StorageManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    cpu_api::CpuExecutionApi::ExecutionManager::Config config{};
    cpu_api::CpuExecutionApi::configure(config);
    manager_.configure({});
  }

  void TearDown() override {
    manager_.shutdown();
    cpu_api::CpuExecutionApi::shutdown();
  }

  storage_manager::StorageManager manager_{};
};

TEST_F(StorageManagerTest, AcquireCpuStorageFromUnifiedManager) {
  storage_manager::CpuStorageRequest request{};
  request.device = cpu::CpuDeviceHandle{0};
  request.size = 256;
  request.alignment = 16;

  auto lease = manager_.acquire(storage_manager::StorageRequest{request});
  ASSERT_TRUE(lease);

  auto *payload = lease.operator->();
  ASSERT_NE(payload, nullptr);
  EXPECT_TRUE(payload->valid());
  EXPECT_NE(payload->tryAs<storage::CpuStorage>(), nullptr);
}

TEST_F(StorageManagerTest, AcquireCpuStorageFromManagerRequestAlias) {
  storage_manager::CpuStorageRequest request{};
  request.device = cpu::CpuDeviceHandle{0};
  request.size = 128;

  storage_manager::StorageManager::Request request_variant{request};
  auto lease = manager_.acquire(request_variant);
  ASSERT_TRUE(lease);

  auto *payload = lease.operator->();
  ASSERT_NE(payload, nullptr);
  EXPECT_TRUE(payload->valid());
  EXPECT_NE(payload->tryAs<storage::CpuStorage>(), nullptr);
}

TEST_F(StorageManagerTest, InvalidRequestThrows) {
  storage_manager::CpuStorageRequest request{};
  request.device = cpu::CpuDeviceHandle{0};
  request.size = 0;

  EXPECT_THROW(manager_.acquire(storage_manager::StorageRequest{request}),
               std::system_error);
}

TEST(StorageManagerStandaloneTest, AcquireWithoutConfigureThrows) {
  storage_manager::StorageManager manager{};
  storage_manager::CpuStorageRequest request{};
  request.device = cpu::CpuDeviceHandle{0};
  request.size = 64;

  EXPECT_THROW(manager.acquire(storage_manager::StorageRequest{request}),
               std::system_error);
}
