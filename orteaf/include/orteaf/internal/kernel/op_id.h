#pragma once

#include <cstdint>
#include <functional>

namespace orteaf::internal::kernel {

/**
 * @brief Operation ID enum for kernel key identification.
 *
 * A strongly-typed enum class based on std::uint64_t for operation
 * identification. Provides type safety for use as kernel keys.
 */
enum class OpId : std::uint64_t {};

} // namespace orteaf::internal::kernel

// Hash support for std::unordered_map and std::unordered_set
namespace std {
template <> struct hash<::orteaf::internal::kernel::OpId> {
  std::size_t
  operator()(const ::orteaf::internal::kernel::OpId &op_id) const noexcept {
    return std::hash<std::uint64_t>{}(static_cast<std::uint64_t>(op_id));
  }
};
} // namespace std
