#pragma once

/**
 * @file typed_storage_manager.inl
 * @brief Implementation of TypedStorageManager template methods.
 */

#include <orteaf/internal/storage/manager/typed_storage_manager.h>

namespace orteaf::internal::storage::manager {

template <typename Storage>
  requires concepts::StorageConcept<Storage>
void TypedStorageManager<Storage>::configure(const Config &config) {
  typename Core::Config core_config{};
  core_config.control_block_capacity = config.control_block_capacity;
  core_config.control_block_block_size = config.control_block_block_size;
  core_config.control_block_growth_chunk_size =
      config.control_block_growth_chunk_size;
  core_config.payload_capacity = config.payload_capacity;
  core_config.payload_block_size = config.payload_block_size;
  core_config.payload_growth_chunk_size = config.payload_growth_chunk_size;

  detail::TypedStorageContext<Storage> context{};
  core_.configure(core_config, context);
}

template <typename Storage>
  requires concepts::StorageConcept<Storage>
typename TypedStorageManager<Storage>::StorageLease
TypedStorageManager<Storage>::acquire(const Request &request) {
  detail::TypedStoragePoolTraits<Storage>::validateRequestOrThrow(request);
  return core_.acquire(request);
}

template <typename Storage>
  requires concepts::StorageConcept<Storage>
void TypedStorageManager<Storage>::shutdown() {
  core_.shutdown();
}

template <typename Storage>
  requires concepts::StorageConcept<Storage>
bool TypedStorageManager<Storage>::isConfigured() const noexcept {
  return core_.isConfigured();
}

} // namespace orteaf::internal::storage::manager
