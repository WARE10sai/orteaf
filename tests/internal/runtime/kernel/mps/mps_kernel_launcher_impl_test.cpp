#include <gtest/gtest.h>

#include "orteaf/internal/runtime/kernel/mps/mps_kernel_launcher_impl.h"

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
