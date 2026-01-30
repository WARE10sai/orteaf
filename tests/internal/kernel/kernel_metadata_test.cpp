#include <gtest/gtest.h>

#include <variant>

#include "orteaf/internal/kernel/core/kernel_entry.h"
#include "orteaf/internal/kernel/core/kernel_metadata.h"

namespace kernel = orteaf::internal::kernel;

namespace {

TEST(KernelMetadataTest, DefaultConstructionIsEmpty) {
  kernel::core::KernelMetadataLease metadata;
  EXPECT_TRUE(
      std::holds_alternative<std::monostate>(metadata.lease()));

  auto entry = metadata.rebuild();
  EXPECT_EQ(entry.execute(), nullptr);
}

TEST(KernelMetadataTest, SetLeaseMonostateKeepsEmpty) {
  kernel::core::KernelMetadataLease metadata;
  metadata.setLease(kernel::core::KernelMetadataLease::Variant{});

  EXPECT_TRUE(
      std::holds_alternative<std::monostate>(metadata.lease()));
  auto entry = metadata.rebuild();
  EXPECT_EQ(entry.execute(), nullptr);
}

} // namespace
