#include "orteaf/internal/runtime/allocator/lowlevel/hierarchical_slot_allocator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "orteaf/internal/backend/backend.h"
#include "orteaf/internal/backend/backend_traits.h"
#include "tests/internal/runtime/allocator/testing/mock_resource.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace allocator = ::orteaf::internal::runtime::allocator;
namespace policies = ::orteaf::internal::runtime::allocator::policies;
using Backend = ::orteaf::internal::backend::Backend;
using Traits = ::orteaf::internal::backend::BackendTraits<Backend::Cpu>;
using BufferView = Traits::BufferView;
using HeapRegion = Traits::HeapRegion;
using ::orteaf::internal::runtime::allocator::testing::MockCpuHeapOps;
using ::orteaf::internal::runtime::allocator::testing::MockCpuHeapOpsImpl;

namespace {

using Allocator = policies::HierarchicalSlotAllocator<MockCpuHeapOps, Backend::Cpu>;

static BufferView MapReturn(HeapRegion region) {
    return BufferView{region.data(), 0, region.size()};
}

class HierarchicalSlotAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockCpuHeapOps::set(&impl_);
    }

    void TearDown() override {
        MockCpuHeapOps::reset();
    }

    NiceMock<MockCpuHeapOpsImpl> impl_;
    MockCpuHeapOps heap_ops_;
    Allocator allocator_;
};

// TODO: Add tests for new API
// - allocate(size) -> BufferView
// - deallocate(BufferView)
// - rs calculation algorithm

}  // namespace
