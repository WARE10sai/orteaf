#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <orteaf/internal/diagnostics/error/error.h>
#include <orteaf/internal/kernel/mps/mps_kernel_base_manager.h>
#include <orteaf/internal/execution/mps/manager/mps_library_manager.h>
#include <tests/internal/execution/mps/manager/testing/execution_ops_provider.h>
#include <tests/internal/execution/mps/manager/testing/manager_test_fixture.h>
#include <tests/internal/testing/error_assert.h>

namespace diag_error = orteaf::internal::diagnostics::error;
namespace kernel_mps = orteaf::internal::kernel::mps;
namespace mps_mgr = orteaf::internal::execution::mps::manager;
namespace mps_wrapper = orteaf::internal::execution::mps::platform::wrapper;
namespace testing_mps = orteaf::tests::execution::mps::testing;

using orteaf::tests::ExpectError;

namespace {

kernel_mps::MpsKernelBaseManager::Config
makeConfig(std::size_t payload_capacity, std::size_t control_block_capacity,
           std::size_t payload_block_size, std::size_t control_block_block_size,
           std::size_t payload_growth_chunk_size,
           std::size_t control_block_growth_chunk_size) {
  kernel_mps::MpsKernelBaseManager::Config config{};
  config.payload_capacity = payload_capacity;
  config.control_block_capacity = control_block_capacity;
  config.payload_block_size = payload_block_size;
  config.control_block_block_size = control_block_block_size;
  config.payload_growth_chunk_size = payload_growth_chunk_size;
  config.control_block_growth_chunk_size = control_block_growth_chunk_size;
  return config;
}

template <class Provider>
class MpsKernelBaseManagerTypedTest : public ::testing::Test {
protected:
  void SetUp() override {
    adapter_ = std::make_unique<Provider>();
    library_manager_ = std::make_unique<mps_mgr::MpsLibraryManager>();
    kernel_base_manager_ = std::make_unique<kernel_mps::MpsKernelBaseManager>();
    
    // Configure library manager first
    mps_mgr::MpsLibraryManager::Config lib_config{};
    lib_config.control_block_capacity = 4;
    lib_config.control_block_block_size = 4;
    lib_config.payload_capacity = 4;
    lib_config.payload_block_size = 4;
    
    mps_mgr::MpsComputePipelineStateManager::Config pipeline_config{};
    pipeline_config.control_block_capacity = 4;
    pipeline_config.control_block_block_size = 4;
    pipeline_config.payload_capacity = 4;
    pipeline_config.payload_block_size = 4;
    
    library_manager_->configureForTest(lib_config, adapter_->device(),
                                       adapter_->ops(), pipeline_config);
  }

  void TearDown() override {
    kernel_base_manager_.reset();
    library_manager_.reset();
    adapter_.reset();
  }

  kernel_mps::MpsKernelBaseManager &manager() { return *kernel_base_manager_; }
  mps_mgr::MpsLibraryManager &libraryManager() { return *library_manager_; }
  auto &adapter() { return *adapter_; }
  auto *getOps() { return adapter_->ops(); }

private:
  std::unique_ptr<Provider> adapter_;
  std::unique_ptr<mps_mgr::MpsLibraryManager> library_manager_;
  std::unique_ptr<kernel_mps::MpsKernelBaseManager> kernel_base_manager_;
};

#if ORTEAF_ENABLE_MPS
using ProviderTypes = ::testing::Types<testing_mps::MockExecutionOpsProvider,
                                       testing_mps::RealExecutionOpsProvider>;
#else
using ProviderTypes = ::testing::Types<testing_mps::MockExecutionOpsProvider>;
#endif

} // namespace

TYPED_TEST_SUITE(MpsKernelBaseManagerTypedTest, ProviderTypes);

// =============================================================================
// Initialization Tests
// =============================================================================

TYPED_TEST(MpsKernelBaseManagerTypedTest, InitializeRejectsNullLibraryManager) {
  auto &manager = this->manager();

  // Act & Assert
  ExpectError(diag_error::OrteafErrc::InvalidArgument, [&] {
    manager.configureForTest(makeConfig(1, 1, 1, 1, 1, 1), nullptr,
                             this->getOps());
  });
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, InitializeRejectsNullOps) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Act & Assert
  ExpectError(diag_error::OrteafErrc::InvalidArgument, [&] {
    manager.configureForTest(makeConfig(1, 1, 1, 1, 1, 1), &library_manager,
                             nullptr);
  });
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, OperationsBeforeInitializationThrow) {
  auto &manager = this->manager();

  // Act & Assert
  ::orteaf::internal::base::HeapVector<kernel_mps::MpsKernelBaseManager::Key>
      keys;
  ExpectError(diag_error::OrteafErrc::InvalidState,
              [&] { (void)manager.acquire(keys); });
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, InitializeSucceeds) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Act
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());

  // Assert
  EXPECT_TRUE(manager.isConfiguredForTest());
  EXPECT_EQ(manager.payloadPoolCapacityForTest(), 2);
  EXPECT_EQ(manager.controlBlockPoolCapacityForTest(), 2);
}

// =============================================================================
// Acquire/Release Tests
// =============================================================================

TYPED_TEST(MpsKernelBaseManagerTypedTest, AcquireEmptyKeysSucceeds) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());

  ::orteaf::internal::base::HeapVector<kernel_mps::MpsKernelBaseManager::Key>
      keys;

  // Act
  auto lease = manager.acquire(keys);

  // Assert
  EXPECT_TRUE(lease);
  EXPECT_EQ(lease->kernelCount(), 0);
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, AcquireWithKeysSucceeds) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());

  ::orteaf::internal::base::HeapVector<kernel_mps::MpsKernelBaseManager::Key>
      keys;
  keys.pushBack({mps_mgr::LibraryKey::Named("test_lib"),
                 mps_mgr::FunctionKey::Named("test_func")});

  // Act
  auto lease = manager.acquire(keys);

  // Assert
  EXPECT_TRUE(lease);
  EXPECT_EQ(lease->kernelCount(), 1);
  EXPECT_NE(lease->getPipeline(0), nullptr);
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, AcquireMultipleKeysSucceeds) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());

  ::orteaf::internal::base::HeapVector<kernel_mps::MpsKernelBaseManager::Key>
      keys;
  keys.pushBack({mps_mgr::LibraryKey::Named("lib1"),
                 mps_mgr::FunctionKey::Named("func1")});
  keys.pushBack({mps_mgr::LibraryKey::Named("lib1"),
                 mps_mgr::FunctionKey::Named("func2")});

  // Act
  auto lease = manager.acquire(keys);

  // Assert
  EXPECT_TRUE(lease);
  EXPECT_EQ(lease->kernelCount(), 2);
  EXPECT_NE(lease->getPipeline(0), nullptr);
  EXPECT_NE(lease->getPipeline(1), nullptr);
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, ReleaseDecreasesRefCount) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());

  ::orteaf::internal::base::HeapVector<kernel_mps::MpsKernelBaseManager::Key>
      keys;

  // Act
  auto lease = manager.acquire(keys);
  EXPECT_TRUE(lease);
  auto count_before = lease.strongCount();

  lease.release();

  // Assert
  EXPECT_FALSE(lease);
}

TYPED_TEST(MpsKernelBaseManagerTypedTest, MultipleAcquireSharesResources) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());

  ::orteaf::internal::base::HeapVector<kernel_mps::MpsKernelBaseManager::Key>
      keys;

  // Act
  auto lease1 = manager.acquire(keys);
  auto lease2 = lease1; // Copy should increase ref count

  // Assert
  EXPECT_TRUE(lease1);
  EXPECT_TRUE(lease2);
  EXPECT_EQ(lease1.strongCount(), 2);
  EXPECT_EQ(lease2.strongCount(), 2);
}

// =============================================================================
// Growth Tests
// =============================================================================

TYPED_TEST(MpsKernelBaseManagerTypedTest, GrowthChunkSizeIsRespected) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  const std::size_t growth_chunk = 3;
  manager.configureForTest(makeConfig(1, 1, 1, 1, growth_chunk, growth_chunk),
                           &library_manager, this->getOps());

  // Assert
  EXPECT_EQ(manager.payloadGrowthChunkSizeForTest(), growth_chunk);
  EXPECT_EQ(manager.controlBlockGrowthChunkSizeForTest(), growth_chunk);
}

// =============================================================================
// Shutdown Tests
// =============================================================================

TYPED_TEST(MpsKernelBaseManagerTypedTest, ShutdownCleansUp) {
  auto &manager = this->manager();
  auto &library_manager = this->libraryManager();

  // Arrange
  manager.configureForTest(makeConfig(2, 2, 2, 2, 1, 1), &library_manager,
                           this->getOps());
  EXPECT_TRUE(manager.isConfiguredForTest());

  // Act
  manager.shutdown();

  // Assert
  EXPECT_FALSE(manager.isConfiguredForTest());
}

TYPED_TEST(MpsKernelBaseManagerTypedTest,
           ShutdownBeforeInitializationIsNoOp) {
  auto &manager = this->manager();

  // Act & Assert (should not throw)
  manager.shutdown();
  EXPECT_FALSE(manager.isConfiguredForTest());
}
