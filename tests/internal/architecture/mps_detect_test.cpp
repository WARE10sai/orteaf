#include "orteaf/internal/architecture/architecture.h"
#include "orteaf/internal/architecture/mps_detect.h"

#include <cstdlib>

#include <gtest/gtest.h>
namespace architecture = orteaf::internal::architecture;

/// Manual test hook: set ORTEAF_EXPECT_MPS_ARCH=m3 (or other ID) to assert your environment.
TEST(MpsDetect, ManualEnvironmentCheck) {
    const char* expected_env = std::getenv("ORTEAF_EXPECT_MPS_ARCH");
    if (!expected_env) {
        GTEST_SKIP() << "Set ORTEAF_EXPECT_MPS_ARCH to run this test on your environment.";
    }

    const char* machine_family = std::getenv("ORTEAF_EXPECT_MPS_METAL_FAMILY");
    const std::string family_hint = machine_family ? machine_family : "m3";
    const char* vendor = std::getenv("ORTEAF_EXPECT_MPS_VENDOR");
    const std::string vendor_hint = vendor ? vendor : "apple";

    const auto arch = architecture::detect_mps_architecture(family_hint, vendor_hint);
    EXPECT_STREQ(expected_env, architecture::IdOf(arch).data());
}

TEST(MpsDetect, MatchesMetalFamily) {
    const auto arch = architecture::detect_mps_architecture("m3", "Apple");
    EXPECT_EQ(arch, architecture::Architecture::mps_m3);
}

TEST(MpsDetect, FallsBackToGenericWhenUnknown) {
    const auto arch = architecture::detect_mps_architecture("unknown_family", "apple");
    EXPECT_EQ(arch, architecture::Architecture::mps_generic);
}
