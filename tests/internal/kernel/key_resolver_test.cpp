#include "orteaf/internal/kernel/core/key_resolver.h"

#include <gtest/gtest.h>

#include <unordered_set>

#include "orteaf/internal/kernel/core/key_components.h"

namespace kernel = orteaf::internal::kernel;
namespace resolver = kernel::key_resolver;
using Architecture = orteaf::internal::architecture::Architecture;
using Execution = orteaf::internal::execution::Execution;
using DType = orteaf::internal::DType;
using Op = orteaf::internal::ops::Op;

// Mock registry for testing
class MockRegistry {
public:
  void add(kernel::KernelKey key) { keys_.insert(key); }

  bool contains(kernel::KernelKey key) const {
    return keys_.find(key) != keys_.end();
  }

private:
  std::unordered_set<kernel::KernelKey> keys_;
};

// ============================================================
// FixedKeyComponents tests
// ============================================================

TEST(KeyComponents, FixedKeyComponentsEquality) {
  kernel::FixedKeyComponents a{static_cast<Op>(1), DType::F32, Execution::Cpu};
  kernel::FixedKeyComponents b{static_cast<Op>(1), DType::F32, Execution::Cpu};
  kernel::FixedKeyComponents c{static_cast<Op>(2), DType::F32, Execution::Cpu};

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

// ============================================================
// VariableKeyComponents tests
// ============================================================

TEST(KeyComponents, VariableKeyComponentsEquality) {
  kernel::VariableKeyComponents a{Architecture::CpuGeneric,
                                  static_cast<kernel::Layout>(0),
                                  static_cast<kernel::Variant>(0)};
  kernel::VariableKeyComponents b{Architecture::CpuGeneric,
                                  static_cast<kernel::Layout>(0),
                                  static_cast<kernel::Variant>(0)};
  kernel::VariableKeyComponents c{Architecture::CpuGeneric,
                                  static_cast<kernel::Layout>(1),
                                  static_cast<kernel::Variant>(0)};

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

// ============================================================
// makeKey tests
// ============================================================

TEST(KeyComponents, MakeKeyCreatesValidKey) {
  kernel::FixedKeyComponents fixed{static_cast<Op>(42), DType::F32,
                                   Execution::Cpu};
  kernel::VariableKeyComponents variable{Architecture::CpuGeneric,
                                         static_cast<kernel::Layout>(3),
                                         static_cast<kernel::Variant>(1)};

  auto key = kernel::makeKey(fixed, variable);

  EXPECT_EQ(kernel::kernel_key::getOp(key), fixed.op);
  EXPECT_EQ(kernel::kernel_key::getDType(key), fixed.dtype);
  EXPECT_EQ(kernel::kernel_key::getArchitecture(key), variable.arch);
  EXPECT_EQ(kernel::kernel_key::getLayout(key), variable.layout);
  EXPECT_EQ(kernel::kernel_key::getVariant(key), variable.variant);
}

// ============================================================
// getCandidates tests
// ============================================================

TEST(KeyResolver, GetCandidatesReturnsCpuArchitectures) {
  kernel::FixedKeyComponents fixed{static_cast<Op>(1), DType::F32,
                                   Execution::Cpu};

  auto candidates = resolver::getCandidates(fixed);

  EXPECT_GT(candidates.size(), 0u);

  // Last candidate should be Generic (fallback)
  EXPECT_EQ(candidates.back().arch, Architecture::CpuGeneric);
}

TEST(KeyResolver, GetCandidatesOrdersSpecificBeforeGeneric) {
  kernel::FixedKeyComponents fixed{static_cast<Op>(1), DType::F32,
                                   Execution::Cpu};

  auto candidates = resolver::getCandidates(fixed);

  // If there are multiple candidates, Generic should be last
  if (candidates.size() > 1) {
    // First candidate should NOT be generic
    EXPECT_NE(candidates.front().arch, Architecture::CpuGeneric);
    // Last candidate should be generic
    EXPECT_EQ(candidates.back().arch, Architecture::CpuGeneric);
  }
}

// ============================================================
// verify tests
// ============================================================

TEST(KeyResolver, VerifyAcceptsValidCandidate) {
  kernel::VariableKeyComponents candidate{Architecture::CpuGeneric,
                                          static_cast<kernel::Layout>(0),
                                          static_cast<kernel::Variant>(0)};
  kernel::KernelArgs args; // Empty args

  // Currently verify always returns true
  EXPECT_TRUE(resolver::verify(candidate, args));
}

// ============================================================
// resolve tests
// ============================================================

TEST(KeyResolver, ResolveFindsRegisteredKey) {
  MockRegistry registry;
  kernel::FixedKeyComponents fixed{static_cast<Op>(1), DType::F32,
                                   Execution::Cpu};
  kernel::VariableKeyComponents variable{Architecture::CpuGeneric,
                                         static_cast<kernel::Layout>(0),
                                         static_cast<kernel::Variant>(0)};

  auto expected_key = kernel::makeKey(fixed, variable);
  registry.add(expected_key);

  kernel::KernelArgs args;
  auto result = resolver::resolve(registry, fixed, args);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, expected_key);
}

TEST(KeyResolver, ResolveReturnsNulloptWhenNotFound) {
  MockRegistry registry; // Empty registry
  kernel::FixedKeyComponents fixed{static_cast<Op>(1), DType::F32,
                                   Execution::Cpu};
  kernel::KernelArgs args;

  auto result = resolver::resolve(registry, fixed, args);

  EXPECT_FALSE(result.has_value());
}

TEST(KeyResolver, ResolvePrefersSpecificOverGeneric) {
  MockRegistry registry;
  kernel::FixedKeyComponents fixed{static_cast<Op>(1), DType::F32,
                                   Execution::Cpu};

  // Register both a specific and generic key
  kernel::VariableKeyComponents specific{Architecture::CpuZen4,
                                         static_cast<kernel::Layout>(0),
                                         static_cast<kernel::Variant>(0)};
  kernel::VariableKeyComponents generic{Architecture::CpuGeneric,
                                        static_cast<kernel::Layout>(0),
                                        static_cast<kernel::Variant>(0)};

  auto specific_key = kernel::makeKey(fixed, specific);
  auto generic_key = kernel::makeKey(fixed, generic);

  registry.add(specific_key);
  registry.add(generic_key);

  kernel::KernelArgs args;
  auto result = resolver::resolve(registry, fixed, args);

  ASSERT_TRUE(result.has_value());
  // Should prefer specific architecture over generic
  EXPECT_EQ(*result, specific_key);
}
