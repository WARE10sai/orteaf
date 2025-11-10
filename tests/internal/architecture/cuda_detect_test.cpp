#include "orteaf/internal/architecture/architecture.h"
#include "orteaf/internal/architecture/cuda_detect.h"
#include "orteaf/internal/backend/backend.h"

#include <cstdlib>

#include <gtest/gtest.h>
namespace architecture = orteaf::internal::architecture;

/// Manual test hook: set ORTEAF_EXPECT_CUDA_ARCH=sm80 (or other ID) to assert your environment.
TEST(CudaDetect, ManualEnvironmentCheck) {
    const char* expected_env = std::getenv("ORTEAF_EXPECT_CUDA_ARCH");
    if (!expected_env) {
        GTEST_SKIP() << "Set ORTEAF_EXPECT_CUDA_ARCH to run this test on your environment.";
    }

    // Developers can override these hints if they want to target a specific GPU.
    const int cc = [] {
        if (const char* cc_env = std::getenv("ORTEAF_EXPECT_CUDA_CC")) {
            return std::atoi(cc_env);
        }
        return 80; // default SM80 for convenience
    }();

    const char* vendor = std::getenv("ORTEAF_EXPECT_CUDA_VENDOR");
    const std::string vendor_hint = vendor ? vendor : "nvidia";

    const auto arch = architecture::detect_cuda_architecture(cc, vendor_hint);
    EXPECT_STREQ(expected_env, architecture::IdOf(arch).data());
}

TEST(CudaDetect, MatchesSm80ViaComputeCapability) {
    const auto arch = architecture::detect_cuda_architecture(80, "NVIDIA");
    EXPECT_EQ(arch, architecture::Architecture::cuda_sm80);
}

TEST(CudaDetect, FallsBackToGenericIfNoMatch) {
    const auto arch = architecture::detect_cuda_architecture(999, "nvidia");
    EXPECT_EQ(arch, architecture::Architecture::cuda_generic);
}
