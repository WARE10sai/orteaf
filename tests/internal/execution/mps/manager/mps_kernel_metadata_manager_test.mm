#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <orteaf/internal/diagnostics/error/error.h>
#include <orteaf/internal/execution/mps/manager/mps_kernel_metadata_manager.h>
#include <orteaf/internal/kernel/kernel_entry.h>
#include <tests/internal/testing/error_assert.h>

namespace diag_error = orteaf::internal::diagnostics::error;
namespace kernel = orteaf::internal::kernel;
namespace mps_manager = orteaf::internal::execution::mps::manager;
using orteaf::tests::ExpectError;

namespace {

kernel::KernelEntry::ExecuteFunc makeExecuteFunc() {
  return [](kernel::KernelEntry::KernelBaseLease &,
            kernel::KernelArgs &) {};
}

mps_manager::MpsKernelMetadataManager::Config makeConfig(
    std::size_t payload_capacity, std::size_t control_block_capacity,
    std::size_t payload_block_size, std::size_t control_block_block_size,
    std::size_t payload_growth_chunk_size,
    std::size_t control_block_growth_chunk_size) {
  mps_manager::MpsKernelMetadataManager::Config config{};
  config.payload_capacity = payload_capacity;
  config.control_block_capacity = control_block_capacity;
  config.payload_block_size = payload_block_size;
  config.control_block_block_size = control_block_block_size;
  config.payload_growth_chunk_size = payload_growth_chunk_size;
  config.control_block_growth_chunk_size = control_block_growth_chunk_size;
  return config;
}

} // namespace

TEST(MpsKernelMetadataManagerTest, InitializeSucceeds) {
  mps_manager::MpsKernelMetadataManager manager;

  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1));

  EXPECT_TRUE(manager.isConfiguredForTest());
  EXPECT_EQ(manager.payloadPoolCapacityForTest(), 2);
  EXPECT_EQ(manager.controlBlockPoolCapacityForTest(), 2);
}

TEST(MpsKernelMetadataManagerTest, OperationsBeforeInitializationThrow) {
  mps_manager::MpsKernelMetadataManager manager;
  ::orteaf::internal::base::HeapVector<mps_manager::MpsKernelMetadataManager::Key>
      keys;

  ExpectError(diag_error::OrteafErrc::InvalidState,
              [&] { (void)manager.acquire(keys, makeExecuteFunc()); });
}

TEST(MpsKernelMetadataManagerTest, AcquireStoresKeysAndExecute) {
  mps_manager::MpsKernelMetadataManager manager;
  manager.configureForTest(makeConfig(0, 1, 1, 1, 1, 1));

  ::orteaf::internal::base::HeapVector<mps_manager::MpsKernelMetadataManager::Key>
      keys;
  keys.pushBack({mps_manager::LibraryKey::Named("lib"),
                 mps_manager::FunctionKey::Named("func")});

  auto execute = makeExecuteFunc();
  auto lease = manager.acquire(keys, execute);

  ASSERT_TRUE(static_cast<bool>(lease));
  auto *payload = lease.operator->();
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(payload->keys().size(), 1u);
  EXPECT_EQ(payload->keys()[0].first.identifier, "lib");
  EXPECT_EQ(payload->keys()[0].second.identifier, "func");
  EXPECT_EQ(payload->execute(), execute);
}
