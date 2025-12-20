#include "orteaf/internal/runtime/base/lease/control_block/weak_shared.h"

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

using WeakSharedCB = ::orteaf::internal::runtime::base::WeakSharedControlBlock<
    PayloadHandle, DummyPayload, DummyPool>;

TEST(WeakSharedControlBlock, BindPayloadStoresHandlePtrAndPool) {
  DummyPool pool{};
  DummyPayload payload{};
  WeakSharedCB cb;
  const PayloadHandle handle{1, 2};

  cb.bindPayload(handle, &payload, &pool);

  EXPECT_TRUE(cb.hasPayload());
  EXPECT_EQ(cb.payloadHandle(), handle);
  EXPECT_EQ(cb.payloadPtr(), &payload);
  EXPECT_EQ(cb.payloadPool(), &pool);
}

TEST(WeakSharedControlBlock, StrongAndWeakCountsBehave) {
  WeakSharedCB cb;

  cb.acquire();
  cb.acquire();
  cb.acquireWeak();
  cb.acquireWeak();

  EXPECT_EQ(cb.count(), 2u);
  EXPECT_EQ(cb.weakCount(), 2u);

  EXPECT_FALSE(cb.release());
  EXPECT_TRUE(cb.release());
  EXPECT_FALSE(cb.releaseWeak());
  EXPECT_TRUE(cb.releaseWeak());

  EXPECT_EQ(cb.count(), 0u);
  EXPECT_EQ(cb.weakCount(), 0u);
}

TEST(WeakSharedControlBlock, TryPromoteDependsOnStrongCount) {
  WeakSharedCB cb;

  EXPECT_FALSE(cb.tryPromote());

  cb.acquire();
  EXPECT_TRUE(cb.tryPromote());
  EXPECT_EQ(cb.count(), 2u);
}

TEST(WeakSharedControlBlock, ClearPayloadResetsPointers) {
  DummyPool pool{};
  DummyPayload payload{};
  WeakSharedCB cb;
  const PayloadHandle handle{3, 4};

  cb.bindPayload(handle, &payload, &pool);
  cb.clearPayload();

  EXPECT_FALSE(cb.hasPayload());
  EXPECT_FALSE(cb.payloadHandle().isValid());
  EXPECT_EQ(cb.payloadPtr(), nullptr);
  EXPECT_EQ(cb.payloadPool(), nullptr);
}

} // namespace
