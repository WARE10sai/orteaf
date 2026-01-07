#pragma once

#include <cstddef>
#include <cstring>

namespace orteaf::internal::execution::cpu::platform {

/**
 * @brief Static inline wrappers for CPU fast-path operations.
 *
 * Unlike MpsFastOps which wraps GPU command buffer operations, CpuFastOps
 * provides minimal CPU operations that might be on the hot path.
 *
 * Currently minimal, but provides a symmetric API with MPS for consistency.
 * Operations can be added here as needed.
 */
struct CpuFastOps {
  /**
   * @brief Copy memory from source to destination.
   *
   * @param dst Destination pointer
   * @param src Source pointer
   * @param size Number of bytes to copy
   */
  static inline void copy(void *dst, const void *src, std::size_t size) {
    std::memcpy(dst, src, size);
  }

  /**
   * @brief Fill memory with a value.
   *
   * @param dst Destination pointer
   * @param value Value to fill (treated as unsigned char)
   * @param size Number of bytes to fill
   */
  static inline void fill(void *dst, int value, std::size_t size) {
    std::memset(dst, value, size);
  }

  /**
   * @brief Zero out memory.
   *
   * @param dst Destination pointer
   * @param size Number of bytes to zero
   */
  static inline void zero(void *dst, std::size_t size) {
    std::memset(dst, 0, size);
  }
};

} // namespace orteaf::internal::execution::cpu::platform
