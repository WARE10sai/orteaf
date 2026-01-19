#include "orteaf/internal/kernel/access.h"
#include "orteaf/internal/kernel/kernel_args.h"
#include "orteaf/internal/kernel/mps/mps_kernel_args.h"

#include <gtest/gtest.h>
#include <type_traits>

namespace kernel = orteaf::internal::kernel;

// ============================================================
// Test Params struct (POD)
// ============================================================

struct MpsTestParams {
  float alpha{0.0f};
  float beta{0.0f};
  int n{0};
};
static_assert(std::is_trivially_copyable_v<MpsTestParams>);

// ============================================================
// MpsKernelArgs tests
// ============================================================

using MpsArgs = kernel::mps::MpsKernelArgs<4, MpsTestParams>;

TEST(MpsKernelArgs, DeviceIsPOD) {
  static_assert(std::is_trivially_copyable_v<MpsArgs::Device>);
  static_assert(std::is_trivially_copyable_v<MpsArgs::Device::Binding>);
}

TEST(MpsKernelArgs, HostDefaultConstruct) {
  MpsArgs::Host host;
  EXPECT_EQ(host.storageCount(), 0);
}

TEST(MpsKernelArgs, DeviceDefaultConstruct) {
  MpsArgs::Device device{};
  EXPECT_EQ(device.binding_count, 0);
  EXPECT_FLOAT_EQ(device.params.alpha, 0.0f);
}

TEST(MpsKernelArgs, ToDeviceBasic) {
  MpsArgs::Host host;
  MpsTestParams params{1.0f, 2.0f, 100};

  auto device = MpsArgs::toDevice(host, params);
  EXPECT_EQ(device.binding_count, 0);
  EXPECT_FLOAT_EQ(device.params.alpha, 1.0f);
  EXPECT_FLOAT_EQ(device.params.beta, 2.0f);
  EXPECT_EQ(device.params.n, 100);
}

// ============================================================
// Type-erased KernelArgs with MPS tests
// ============================================================

using TypeErasedArgs = kernel::KernelArgs<4, MpsTestParams>;

TEST(KernelArgsMps, EraseFromMpsKernelArgs) {
  MpsArgs mps_args;
  TypeErasedArgs args = TypeErasedArgs::erase(std::move(mps_args));
  EXPECT_TRUE(args.valid());
}

TEST(KernelArgsMps, TryAsMpsKernelArgs) {
  MpsArgs mps_args;
  TypeErasedArgs args = TypeErasedArgs::erase(std::move(mps_args));

  auto *ptr = args.tryAs<MpsArgs>();
  EXPECT_NE(ptr, nullptr);
}

TEST(KernelArgsMps, ExecutionReturnsCorrectBackend) {
  MpsArgs mps_args;
  TypeErasedArgs args = TypeErasedArgs::erase(std::move(mps_args));

  EXPECT_EQ(args.execution(), orteaf::internal::execution::Execution::Mps);
}

TEST(KernelArgsMps, VisitPattern) {
  MpsArgs mps_args;
  TypeErasedArgs args = TypeErasedArgs::erase(std::move(mps_args));

  bool visited_mps = false;
  args.visit([&](auto &ka) {
    using T = std::decay_t<decltype(ka)>;
    if constexpr (std::is_same_v<T, MpsArgs>) {
      visited_mps = true;
    }
  });

  EXPECT_TRUE(visited_mps);
}

TEST(KernelArgsMps, TryAsWrongTypeReturnsNull) {
  using CpuArgs = kernel::cpu::CpuKernelArgs<4, MpsTestParams>;

  MpsArgs mps_args;
  TypeErasedArgs args = TypeErasedArgs::erase(std::move(mps_args));

  // Trying to get as CPU should return nullptr
  auto *ptr = args.tryAs<CpuArgs>();
  EXPECT_EQ(ptr, nullptr);
}
