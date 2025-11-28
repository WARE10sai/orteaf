#include "orteaf/internal/runtime/allocator/resource/mps/mps_resource.h"

#if ORTEAF_ENABLE_MPS

namespace orteaf::internal::backend::mps {

void MpsResource::initialize(const Config& config) noexcept {
    device_ = config.device;
    heap_ = config.heap;
    usage_ = config.usage;
    initialized_ = (device_ != nullptr && heap_ != nullptr);
}

MpsResource::BufferView MpsResource::allocate(std::size_t size, std::size_t /*alignment*/) {
    if (!initialized_ || size == 0) {
        return {};
    }

    MPSBuffer_t buffer = createBuffer(heap_, size, usage_);
    if (!buffer) {
        return {};
    }

    return BufferView{buffer, 0, size};
}

void MpsResource::deallocate(BufferView view, std::size_t /*size*/, std::size_t /*alignment*/) noexcept {
    if (!view) {
        return;
    }
    destroyBuffer(view.raw());
}

}  // namespace orteaf::internal::backend::mps

#endif  // ORTEAF_ENABLE_MPS
