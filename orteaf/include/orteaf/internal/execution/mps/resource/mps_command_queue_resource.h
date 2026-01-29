#pragma once

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/execution/mps/manager/mps_fence_lifetime_manager.h"
#include "orteaf/internal/execution/mps/mps_handles.h"
#include "orteaf/internal/execution/mps/platform/wrapper/mps_command_queue.h"

namespace orteaf {
namespace internal {
namespace execution {
namespace mps {
namespace manager {
struct CommandQueuePayloadPoolTraits;
} // namespace manager
} // namespace mps
} // namespace execution
} // namespace internal
} // namespace orteaf

namespace orteaf::internal::execution::mps::resource {

class MpsCommandQueueResource {
public:
  using CommandQueueType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsCommandQueue_t;
  using CommandQueueHandle =
      ::orteaf::internal::execution::mps::MpsCommandQueueHandle;
  using FenceLifetimeManager =
      ::orteaf::internal::execution::mps::manager::MpsFenceLifetimeManager;

  MpsCommandQueueResource() = default;
  MpsCommandQueueResource(const MpsCommandQueueResource &) = delete;
  MpsCommandQueueResource &operator=(const MpsCommandQueueResource &) = delete;
  MpsCommandQueueResource(MpsCommandQueueResource &&) = default;
  MpsCommandQueueResource &operator=(MpsCommandQueueResource &&) = default;
  ~MpsCommandQueueResource() = default;

  CommandQueueType queue() const noexcept { return queue_; }
  bool hasQueue() const noexcept { return queue_ != nullptr; }

  FenceLifetimeManager &lifetime() noexcept { return lifetime_; }
  const FenceLifetimeManager &lifetime() const noexcept { return lifetime_; }

#if ORTEAF_ENABLE_TEST
  void setQueueForTest(CommandQueueType queue) noexcept { setQueue(queue); }
#endif

private:
  void setQueue(CommandQueueType queue) noexcept { queue_ = queue; }

  CommandQueueType queue_{nullptr};
  FenceLifetimeManager lifetime_{};

  friend struct ::orteaf::internal::execution::mps::manager::
      CommandQueuePayloadPoolTraits;
};

} // namespace orteaf::internal::execution::mps::resource

#endif // ORTEAF_ENABLE_MPS
