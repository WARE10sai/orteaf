#pragma once

#include <utility>

#include <orteaf/internal/execution/allocator/resource/mps/mps_resource.h>
#include <orteaf/internal/execution/mps/manager/mps_buffer_manager.h>
#include <orteaf/internal/execution/mps/resource/mps_fence_token.h>
#include <orteaf/internal/storage/mps/mps_storage_layout.h>

namespace orteaf::internal::storage::mps {
class MpsStorage {
public:
  using MpsResource = ::orteaf::internal::execution::allocator::resource::mps::MpsResource;
  using BufferLease =
      ::orteaf::internal::execution::mps::manager::MpsBufferManager<MpsResource>::StrongBufferLease;
  // TODO: Re-enable fence tokens after revisiting MpsFenceToken ownership/copy rules.
  // using FenceToken = ::orteaf::internal::execution::mps::resource::MpsFenceToken;
  using Layout = ::orteaf::internal::storage::mps::MpsStorageLayout;

  struct Config {
    BufferLease buffer_lease{};
    Layout layout{};
    // TODO: Add fence_token once MpsFenceToken is updated.
    // FenceToken fence_token{};
  };

  MpsStorage() = default;
  explicit MpsStorage(Config config)
      : buffer_lease_(std::move(config.buffer_lease)),
        layout_(std::move(config.layout)) {}

  MpsStorage(const MpsStorage &) = default;
  MpsStorage &operator=(const MpsStorage &) = default;
  MpsStorage(MpsStorage &&) = default;
  MpsStorage &operator=(MpsStorage &&) = default;
  ~MpsStorage() = default;

private:
    BufferLease buffer_lease_;
    // TODO: Re-enable once MpsFenceToken is updated.
    // FenceToken fence_token_{};
    Layout layout_;
};
} // namespace orteaf::internal::storage::mps
