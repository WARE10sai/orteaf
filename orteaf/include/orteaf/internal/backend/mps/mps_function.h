/**
 * @file mps_function.h
 * @brief MPS/Metal function creation and destruction helpers.
 */
#pragma once

#include <string_view>

struct MPSFunction_st; using MPSFunction_t = MPSFunction_st*;

#include "orteaf/internal/backend/mps/mps_library.h"

static_assert(sizeof(MPSFunction_t) == sizeof(void*), "MPSFunction must be pointer-sized.");

namespace orteaf::internal::backend::mps {

/**
 * @brief Create a Metal function by name from a library.
 * @param library Opaque library handle
 * @param name Function name (UTF-8)
 * @return Opaque function handle, or nullptr when unavailable/disabled.
 */
MPSFunction_t create_function(MPSLibrary_t library, std::string_view name);

/**
 * @brief Destroy a Metal function; ignores nullptr.
 */
void destroy_function(MPSFunction_t function);

} // namespace orteaf::internal::backend::mps