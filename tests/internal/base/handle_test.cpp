#include "orteaf/internal/base/handle.h"

#include <gtest/gtest.h>

namespace base = orteaf::internal::base;

TEST(Handle, BasicComparisonAndConversion) {
    base::StreamHandle stream1{3};
    base::StreamHandle stream2{3};
    base::StreamHandle stream3{4};

    EXPECT_EQ(stream1, stream2);
    EXPECT_NE(stream1, stream3);
    EXPECT_EQ(static_cast<uint32_t>(stream1), 3u); // Handle casts to index type (uint32_t)
    EXPECT_LT(stream1, stream3);
    EXPECT_TRUE(stream1.isValid());
}

TEST(Handle, InvalidHelper) {
    constexpr auto bad = base::ContextHandle::invalid();
    EXPECT_FALSE(bad.isValid());
    EXPECT_EQ(static_cast<uint32_t>(bad), base::ContextHandle::invalid_index());
}

TEST(Handle, DeviceTypeIsIndependent) {
    base::DeviceHandle device{0};
    base::StreamHandle stream{0};
    // Ensure there is no implicit conversion between different Handle tags.
    static_assert(!std::is_convertible_v<base::StreamHandle, base::DeviceHandle>);
    static_assert(!std::is_convertible_v<base::DeviceHandle, base::StreamHandle>);
    EXPECT_EQ(static_cast<uint32_t>(device), 0u);
    EXPECT_EQ(static_cast<uint32_t>(stream), 0u);
}
