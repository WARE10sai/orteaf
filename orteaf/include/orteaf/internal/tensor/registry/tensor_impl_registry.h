#pragma once

/**
 * @file tensor_impl_registry.h
 * @brief Core registry template for tensor implementations.
 *
 * This file contains the TensorImplRegistry template class which provides
 * pool management for multiple TensorImpl types. This is internal
 * infrastructure - contributors should not modify this file.
 *
 * @see extension/tensor/registry/tensor_impl_types.h for contributor-editable
 *      registration of new TensorImpl types.
 */

#include <tuple>
#include <type_traits>

#include <orteaf/internal/storage/manager/storage_manager.h>

namespace orteaf::internal::tensor::registry {

// =============================================================================
// TensorImplTraits - Forward declaration
// =============================================================================

/**
 * @brief Traits for a tensor implementation.
 *
 * This must be specialized for each TensorImpl type.
 * Specializations should be defined in extension/tensor/registry/.
 *
 * @tparam Impl The TensorImpl type
 */
template <typename Impl> struct TensorImplTraits;

// =============================================================================
// TensorImplRegistry - Core template
// =============================================================================

/**
 * @brief Registry holding managers for multiple TensorImpl types.
 *
 * Automatically creates and manages all registered TensorImpl managers.
 * This is internal infrastructure that should not be modified by contributors.
 *
 * @tparam Impls Variadic list of TensorImpl types
 */
template <typename... Impls> class TensorImplRegistry {
public:
  using StorageManager = ::orteaf::internal::storage::manager::StorageManager;
  using ManagerTuple = std::tuple<typename TensorImplTraits<Impls>::Manager...>;

  struct Config {
    std::tuple<typename TensorImplTraits<Impls>::Manager::Config...> configs{};

    template <typename Impl> auto &get() {
      return std::get<typename TensorImplTraits<Impl>::Manager::Config>(
          configs);
    }

    template <typename Impl> const auto &get() const {
      return std::get<typename TensorImplTraits<Impl>::Manager::Config>(
          configs);
    }
  };

  TensorImplRegistry() = default;
  TensorImplRegistry(const TensorImplRegistry &) = delete;
  TensorImplRegistry &operator=(const TensorImplRegistry &) = delete;
  TensorImplRegistry(TensorImplRegistry &&) = default;
  TensorImplRegistry &operator=(TensorImplRegistry &&) = default;

  void configure(const Config &config, StorageManager &storage_manager) {
    configureImpl<Impls...>(config, storage_manager);
  }

  void shutdown() { shutdownImpl<Impls...>(); }

  bool isConfigured() const noexcept { return isConfiguredImpl<Impls...>(); }

  template <typename Impl> auto &get() {
    return std::get<typename TensorImplTraits<Impl>::Manager>(managers_);
  }

  template <typename Impl> const auto &get() const {
    return std::get<typename TensorImplTraits<Impl>::Manager>(managers_);
  }

private:
  template <typename First, typename... Rest>
  void configureImpl(const Config &config, StorageManager &storage_manager) {
    std::get<typename TensorImplTraits<First>::Manager>(managers_).configure(
        std::get<typename TensorImplTraits<First>::Manager::Config>(
            config.configs),
        storage_manager);
    if constexpr (sizeof...(Rest) > 0) {
      configureImpl<Rest...>(config, storage_manager);
    }
  }

  template <typename First, typename... Rest> void shutdownImpl() {
    std::get<typename TensorImplTraits<First>::Manager>(managers_).shutdown();
    if constexpr (sizeof...(Rest) > 0) {
      shutdownImpl<Rest...>();
    }
  }

  template <typename First, typename... Rest>
  bool isConfiguredImpl() const noexcept {
    bool first = std::get<typename TensorImplTraits<First>::Manager>(managers_)
                     .isConfigured();
    if constexpr (sizeof...(Rest) > 0) {
      return first && isConfiguredImpl<Rest...>();
    }
    return first;
  }

  ManagerTuple managers_{};
};

} // namespace orteaf::internal::tensor::registry
