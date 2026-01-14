#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

#include <orteaf/internal/base/lease/control_block/strong.h>
#include <orteaf/internal/base/manager/pool_manager.h>
#include <orteaf/internal/base/pool/slot_pool.h>
#include <orteaf/internal/diagnostics/error/error.h>
#include <orteaf/internal/storage/manager/storage_request.h>
#include <orteaf/internal/storage/storage.h>
#include <orteaf/internal/storage/storage_handles.h>

namespace orteaf::internal::storage::manager {

namespace detail {

struct StoragePayloadPoolTraits {
  using Payload = ::orteaf::internal::storage::Storage;
  using Handle = ::orteaf::internal::storage::StorageHandle;
  using Request = StorageRequest;
  struct Context {};

  static constexpr bool destroy_on_release = true;
  static constexpr const char *ManagerName = "Storage manager";

  static void validateRequestOrThrow(const Request &request) {
    std::visit(
        [](const auto &req) {
          using RequestT = std::decay_t<decltype(req)>;
          if constexpr (std::is_same_v<RequestT, CpuStorageRequest>) {
            if (!req.device.isValid()) {
              ::orteaf::internal::diagnostics::error::throwError(
                  ::orteaf::internal::diagnostics::error::OrteafErrc::
                      InvalidArgument,
                  "CpuStorage request requires a valid device handle");
            }
            if (req.size == 0) {
              ::orteaf::internal::diagnostics::error::throwError(
                  ::orteaf::internal::diagnostics::error::OrteafErrc::
                      InvalidArgument,
                  "CpuStorage request size must be > 0");
            }
          }
#if ORTEAF_ENABLE_MPS
          else if constexpr (std::is_same_v<RequestT, MpsStorageRequest>) {
            if (!req.device.isValid()) {
              ::orteaf::internal::diagnostics::error::throwError(
                  ::orteaf::internal::diagnostics::error::OrteafErrc::
                      InvalidArgument,
                  "MpsStorage request requires a valid device handle");
            }
            if (req.size == 0) {
              ::orteaf::internal::diagnostics::error::throwError(
                  ::orteaf::internal::diagnostics::error::OrteafErrc::
                      InvalidArgument,
                  "MpsStorage request size must be > 0");
            }
          }
#endif // ORTEAF_ENABLE_MPS
        },
        request);
  }

  static bool create(Payload &payload, const Request &request,
                     const Context &) {
    return std::visit(
        [&](const auto &req) -> bool {
          using RequestT = std::decay_t<decltype(req)>;
          if constexpr (std::is_same_v<RequestT, CpuStorageRequest>) {
            auto storage =
                ::orteaf::internal::storage::cpu::CpuStorage::builder()
                    .withDeviceHandle(req.device)
                    .withSize(req.size)
                    .withAlignment(req.alignment)
                    .withLayout(req.layout)
                    .build();
            payload = Payload::erase(std::move(storage));
            return true;
          }
#if ORTEAF_ENABLE_MPS
          else if constexpr (std::is_same_v<RequestT, MpsStorageRequest>) {
            auto storage =
                ::orteaf::internal::storage::mps::MpsStorage::builder()
                    .withDeviceHandle(req.device, req.heap_key)
                    .withSize(req.size)
                    .withAlignment(req.alignment)
                    .withLayout(req.layout)
                    .build();
            payload = Payload::erase(std::move(storage));
            return true;
          }
#endif // ORTEAF_ENABLE_MPS
          else {
            return false;
          }
        },
        request);
  }

  static void destroy(Payload &payload, const Request &, const Context &) {
    payload = Payload{};
  }
};

} // namespace detail

class StorageManager {
public:
  using PayloadPool = ::orteaf::internal::base::pool::SlotPool<
      detail::StoragePayloadPoolTraits>;
  using ControlBlock = ::orteaf::internal::base::StrongControlBlock<
      ::orteaf::internal::storage::StorageHandle,
      ::orteaf::internal::storage::Storage, PayloadPool>;

  struct Traits {
    using PayloadPool = StorageManager::PayloadPool;
    using ControlBlock = StorageManager::ControlBlock;
    struct ControlBlockTag {};
    using PayloadHandle = ::orteaf::internal::storage::StorageHandle;
    static constexpr const char *Name =
        detail::StoragePayloadPoolTraits::ManagerName;
  };

  using Core = ::orteaf::internal::base::PoolManager<Traits>;
  using StorageLease = Core::StrongLeaseType;
  using Request = detail::StoragePayloadPoolTraits::Request;
  using Context = detail::StoragePayloadPoolTraits::Context;

  struct Config {
    std::size_t control_block_capacity{0};
    std::size_t control_block_block_size{0};
    std::size_t control_block_growth_chunk_size{1};
    std::size_t payload_capacity{0};
    std::size_t payload_block_size{0};
    std::size_t payload_growth_chunk_size{1};
  };

  StorageManager() = default;
  StorageManager(const StorageManager &) = delete;
  StorageManager &operator=(const StorageManager &) = delete;
  StorageManager(StorageManager &&) = default;
  StorageManager &operator=(StorageManager &&) = default;
  ~StorageManager() = default;

  void configure(const Config &config) {
    std::size_t payload_capacity = config.payload_capacity;
    if (payload_capacity == 0) {
      payload_capacity = 64;
    }
    std::size_t payload_block_size = config.payload_block_size;
    if (payload_block_size == 0) {
      payload_block_size = 16;
    }
    std::size_t control_block_capacity = config.control_block_capacity;
    if (control_block_capacity == 0) {
      control_block_capacity = 64;
    }
    std::size_t control_block_block_size = config.control_block_block_size;
    if (control_block_block_size == 0) {
      control_block_block_size = 16;
    }

    Request request{};
    Context context{};

    Core::Builder<Request, Context> builder{};
    builder.withControlBlockCapacity(control_block_capacity)
        .withControlBlockBlockSize(control_block_block_size)
        .withControlBlockGrowthChunkSize(
            config.control_block_growth_chunk_size)
        .withPayloadCapacity(payload_capacity)
        .withPayloadBlockSize(payload_block_size)
        .withPayloadGrowthChunkSize(config.payload_growth_chunk_size)
        .withRequest(request)
        .withContext(context)
        .configure(core_);
  }

  StorageLease acquire(const Request &request) {
    core_.ensureConfigured();
    detail::StoragePayloadPoolTraits::validateRequestOrThrow(request);

    Context context{};

    auto payload_handle = core_.reserveUncreatedPayloadOrGrow();
    if (!payload_handle.isValid()) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
          "Storage manager has no available slots");
    }

    if (!core_.emplacePayload(payload_handle, request, context)) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
          "Storage manager failed to create storage");
    }

    return core_.acquireStrongLease(payload_handle);
  }

  void shutdown() {
    Request request{};
    Context context{};
    core_.shutdown(request, context);
  }

  bool isConfigured() const noexcept { return core_.isConfigured(); }

private:
  Core core_{};
};

} // namespace orteaf::internal::storage::manager
