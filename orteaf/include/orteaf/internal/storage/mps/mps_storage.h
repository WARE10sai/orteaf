#pragma once

#include <orteaf/internal/execution/allocator/resource/mps/mps_resource.h>
#include <orteaf/internal/execution/mps/manager/mps_buffer_manager.h>
#include <orteaf/internal/execution/mps/resource/mps_fence_token.h>
#include <orteaf/internal/storage/mps/mps_storage_layout.h>

namespace orteaf::internal::storage::mps {
class MpsStorage {
public:
  using MpsResource = ::orteaf::internal::execution::allocator::resource::mps::MpsResource;
  using StrongBufferLease =
      ::orteaf::internal::execution::mps::manager::MpsBufferManager<MpsResource>::StrongBufferLease;
  using FenceToken = ::orteaf::internal::execution::mps::resource::MpsFenceToken;
  using Layout = ::orteaf::internal::storage::mps::MpsStorageLayout;

private:
    StrongBufferLease strong_buffer_lease_;
    FenceToken fence_token_;
    Layout layout_;
};
} // namespace orteaf::internal::storage::mps
