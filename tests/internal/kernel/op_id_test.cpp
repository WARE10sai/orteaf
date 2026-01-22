#include "orteaf/internal/kernel/op_id.h"

#include <gtest/gtest.h>

#include <unordered_map>
#include <unordered_set>

namespace kernel = orteaf::internal::kernel;

// ============================================================
// OpId basic functionality tests
// ============================================================

TEST(OpId, DefaultConstructedIsZero) {
  kernel::OpId op_id{};
  EXPECT_EQ(static_cast<std::uint64_t>(op_id), 0);
}

TEST(OpId, ExplicitConstructionWithValue) {
  kernel::OpId op_id{static_cast<kernel::OpId>(42)};
  EXPECT_EQ(static_cast<std::uint64_t>(op_id), 42);
}

TEST(OpId, StaticCastFromInteger) {
  auto op_id = static_cast<kernel::OpId>(123);
  EXPECT_EQ(static_cast<std::uint64_t>(op_id), 123);
}

TEST(OpId, StaticCastToInteger) {
  kernel::OpId op_id = static_cast<kernel::OpId>(456);
  std::uint64_t value = static_cast<std::uint64_t>(op_id);
  EXPECT_EQ(value, 456);
}

// ============================================================
// OpId comparison operator tests
// ============================================================

TEST(OpId, EqualityOperator) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id3 = static_cast<kernel::OpId>(200);

  EXPECT_TRUE(op_id1 == op_id2);
  EXPECT_FALSE(op_id1 == op_id3);
}

TEST(OpId, InequalityOperator) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id3 = static_cast<kernel::OpId>(200);

  EXPECT_FALSE(op_id1 != op_id2);
  EXPECT_TRUE(op_id1 != op_id3);
}

TEST(OpId, LessThanOperator) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(200);

  EXPECT_TRUE(op_id1 < op_id2);
  EXPECT_FALSE(op_id2 < op_id1);
  EXPECT_FALSE(op_id1 < op_id1);
}

TEST(OpId, LessThanOrEqualOperator) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(200);
  kernel::OpId op_id3 = static_cast<kernel::OpId>(100);

  EXPECT_TRUE(op_id1 <= op_id2);
  EXPECT_TRUE(op_id1 <= op_id3);
  EXPECT_FALSE(op_id2 <= op_id1);
}

TEST(OpId, GreaterThanOperator) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(200);

  EXPECT_TRUE(op_id2 > op_id1);
  EXPECT_FALSE(op_id1 > op_id2);
  EXPECT_FALSE(op_id1 > op_id1);
}

TEST(OpId, GreaterThanOrEqualOperator) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(200);
  kernel::OpId op_id3 = static_cast<kernel::OpId>(100);

  EXPECT_TRUE(op_id2 >= op_id1);
  EXPECT_TRUE(op_id1 >= op_id3);
  EXPECT_FALSE(op_id1 >= op_id2);
}

// ============================================================
// OpId hash support tests
// ============================================================

TEST(OpId, HashSupport) {
  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id3 = static_cast<kernel::OpId>(200);

  std::hash<kernel::OpId> hasher;

  // Same values should produce same hash
  EXPECT_EQ(hasher(op_id1), hasher(op_id2));

  // Different values should (very likely) produce different hashes
  // Note: Hash collisions are theoretically possible but extremely unlikely
  EXPECT_NE(hasher(op_id1), hasher(op_id3));
}

TEST(OpId, UnorderedSetUsage) {
  std::unordered_set<kernel::OpId> op_id_set;

  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(200);
  kernel::OpId op_id3 = static_cast<kernel::OpId>(100); // Same as op_id1

  op_id_set.insert(op_id1);
  op_id_set.insert(op_id2);
  op_id_set.insert(op_id3); // Should not increase size

  EXPECT_EQ(op_id_set.size(), 2);
  EXPECT_TRUE(op_id_set.count(op_id1) > 0);
  EXPECT_TRUE(op_id_set.count(op_id2) > 0);
  EXPECT_TRUE(op_id_set.count(op_id3) > 0); // Same as op_id1
}

TEST(OpId, UnorderedMapUsage) {
  std::unordered_map<kernel::OpId, std::string> op_id_map;

  kernel::OpId op_id1 = static_cast<kernel::OpId>(100);
  kernel::OpId op_id2 = static_cast<kernel::OpId>(200);

  op_id_map[op_id1] = "Operation 100";
  op_id_map[op_id2] = "Operation 200";

  EXPECT_EQ(op_id_map.size(), 2);
  EXPECT_EQ(op_id_map[op_id1], "Operation 100");
  EXPECT_EQ(op_id_map[op_id2], "Operation 200");

  // Overwrite existing key
  op_id_map[op_id1] = "Updated Operation 100";
  EXPECT_EQ(op_id_map.size(), 2);
  EXPECT_EQ(op_id_map[op_id1], "Updated Operation 100");
}

// ============================================================
// OpId constexpr tests
// ============================================================

TEST(OpId, ConstexprSupport) {
  constexpr kernel::OpId op_id1{};
  static_assert(static_cast<std::uint64_t>(op_id1) == 0);

  constexpr kernel::OpId op_id2 = static_cast<kernel::OpId>(42);
  static_assert(static_cast<std::uint64_t>(op_id2) == 42);

  constexpr kernel::OpId op_id3 = static_cast<kernel::OpId>(42);
  static_assert(op_id2 == op_id3);
  static_assert(!(op_id2 != op_id3));
}
