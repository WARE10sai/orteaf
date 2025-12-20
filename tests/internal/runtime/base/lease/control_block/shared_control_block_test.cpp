#include "orteaf/internal/runtime/base/lease/control_block/shared.h"

#include <gtest/gtest.h>

#include "orteaf/internal/base/handle.h"

namespace {

struct PayloadTag {};
using PayloadHandle = ::orteaf::internal::base::Handle<PayloadTag, std::uint32_t, std::uint8_t>;

struct DummyPayload {
  int value{0};
};

struct DummyPool {
  int marker{0};
};

using SharedCB = ::orteaf::internal::runtime::base::SharedControlBlock<
    PayloadHandle, DummyPayload, DummyPool>;

TEST(SharedControlBlock, BindPayloadStoresHandlePtrAndPool) {
  DummyPool pool{};
  DummyPayload payload{};
  SharedCB cb;
  const PayloadHandle handle{1, 2};

  cb.bindPayload(handle, &payload, &pool);

  EXPECT_TRUE(cb.hasPayload());
  EXPECT_EQ(cb.payloadHandle(), handle);
  EXPECT_EQ(cb.payloadPtr(), &payload);
  EXPECT_EQ(cb.payloadPool(), &pool);
}

TEST(SharedControlBlock, StrongCountIncrementsAndReleaseSignalsLast) {
  SharedCB cb;

  EXPECT_EQ(cb.count(), 0u);
  cb.acquire();
  cb.acquire();
  EXPECT_EQ(cb.count(), 2u);

  EXPECT_FALSE(cb.release());
  EXPECT_TRUE(cb.release());
  EXPECT_EQ(cb.count(), 0u);
}

TEST(SharedControlBlock, ClearPayloadResetsPointers) {
  DummyPool pool{};
  DummyPayload payload{};
  SharedCB cb;
  const PayloadHandle handle{3, 4};

  cb.bindPayload(handle, &payload, &pool);
  cb.clearPayload();

  EXPECT_FALSE(cb.hasPayload());
  EXPECT_FALSE(cb.payloadHandle().isValid());
  EXPECT_EQ(cb.payloadPtr(), nullptr);
  EXPECT_EQ(cb.payloadPool(), nullptr);
}

} // namespace
