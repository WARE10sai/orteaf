#include "orteaf/internal/kernel/core/key_resolver.h"

#include <orteaf/internal/architecture/architecture.h>

namespace orteaf::internal::kernel::key_resolver {

namespace arch = ::orteaf::internal::architecture;

base::SmallVector<VariableKeyComponents, 8>
getCandidates(const FixedKeyComponents &fixed) {
  base::SmallVector<VariableKeyComponents, 8> candidates;

  // Get all architectures for this execution backend
  auto architectures = arch::architecturesOf(fixed.execution);

  // Add candidates in reverse order (specific architectures first, then
  // generic) For each architecture, add Default layout and variant first
  // TODO: Expand with more Layout/Variant combinations based on Op

  // Specific architectures first (skip index 0 which is Generic)
  for (std::size_t i = architectures.size(); i > 1; --i) {
    auto architecture = architectures[i - 1];
    candidates.pushBack({
        architecture,
        static_cast<Layout>(0), // Default layout
        static_cast<Variant>(0) // Default variant
    });
  }

  // Generic architecture last (fallback)
  if (!architectures.empty()) {
    auto generic = architectures[0]; // Index 0 is always Generic
    candidates.pushBack({
        generic,
        static_cast<Layout>(0),
        static_cast<Variant>(0),
    });
  }

  return candidates;
}

bool verify(const VariableKeyComponents & /*candidate*/,
            const KernelArgs & /*args*/) {
  // TODO: Implement actual verification logic
  // For now, always return true (accept all candidates)
  // Future: Check layout compatibility, tensor contiguity, etc.
  return true;
}

} // namespace orteaf::internal::kernel::key_resolver
