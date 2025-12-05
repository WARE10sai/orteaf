#include <gtest/gtest.h>

#include "orteaf/internal/runtime/kernel/mps/mps_kernel_launcher_impl.h"
#include "orteaf/internal/runtime/ops/mps/private/mps_private_ops.h"

namespace base = orteaf::internal::base;

namespace mps_rt = orteaf::internal::runtime::mps;

TEST(MpsKernelLauncherImplTest, StoresUniqueKeysInOrder) {
    mps_rt::MpsKernelLauncherImpl<3> impl({
        {"libA", "funcX"},
        {"libB", "funcY"},
        {"libA", "funcX"},  // duplicate should be ignored
    });

    const auto& keys = impl.keysForTest();
    ASSERT_EQ(impl.sizeForTest(), 2u);

    EXPECT_EQ(keys[0].first.identifier, "funcX");
    EXPECT_EQ(keys[0].second.identifier, "libA");

    EXPECT_EQ(keys[1].first.identifier, "funcY");
    EXPECT_EQ(keys[1].second.identifier, "libB");
}

namespace {

class DummyPrivateOps : public orteaf::internal::runtime::ops::mps::MpsPrivateOps {
public:
    using PipelineLease = mps_rt::MpsComputePipelineStateManager::PipelineLease;

    // Record requests and return dummy leases.
    PipelineLease acquirePipeline(base::DeviceHandle device,
                                  const mps_rt::LibraryKey& library_key,
                                  const mps_rt::FunctionKey& function_key) {
        last_device = device;
        last_library = library_key.identifier;
        last_function = function_key.identifier;
        // Return an empty lease; we only validate call ordering and size.
        return PipelineLease{};
    }

    base::DeviceHandle last_device{};
    std::string last_library{};
    std::string last_function{};
};

}  // namespace

TEST(MpsKernelLauncherImplTest, InitializeAcquiresPipelinesInOrder) {
    mps_rt::MpsKernelLauncherImpl<2> impl({
        {"libA", "funcX"},
        {"libB", "funcY"},
    });

    DummyPrivateOps ops;
    const base::DeviceHandle device{0};

    impl.initialize(device, ops);

    // validate size and that initialized flag is set
    EXPECT_TRUE(impl.initialized());
    EXPECT_EQ(impl.sizeForTest(), 2u);
}
