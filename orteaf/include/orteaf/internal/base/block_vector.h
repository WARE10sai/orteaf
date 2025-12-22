#pragma once

#include <algorithm>
#include <cstddef>
#include <new>
#include <type_traits>

#include "orteaf/internal/base/heap_vector.h"

namespace orteaf::internal::base {

/**
 * @brief Segmented vector with stable element addresses on grow.
 *
 * BlockVector stores elements in fixed-size blocks allocated independently,
 * so growing the container never relocates existing elements. This is useful
 * when external code holds raw pointers into the storage.
 */
template <typename T, std::size_t BlockSize = 64>
class BlockVector {
public:
  static_assert(BlockSize > 0, "BlockSize must be > 0");

  BlockVector() noexcept = default;
  BlockVector(const BlockVector &) = delete;
  BlockVector &operator=(const BlockVector &) = delete;
  BlockVector(BlockVector &&other) noexcept
      : blocks_(std::move(other.blocks_)),
        size_(other.size_),
        capacity_(other.capacity_) {
    other.size_ = 0;
    other.capacity_ = 0;
  }
  BlockVector &operator=(BlockVector &&other) noexcept {
    if (this == &other) {
      return *this;
    }
    destroyElements();
    releaseBlocks();
    blocks_ = std::move(other.blocks_);
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.size_ = 0;
    other.capacity_ = 0;
    return *this;
  }
  ~BlockVector() {
    destroyElements();
    releaseBlocks();
  }

  void resize(std::size_t new_size) {
    if (new_size < size_) {
      destroyRange(new_size, size_);
      size_ = new_size;
      return;
    }
    ensureCapacityFor(new_size);
    std::size_t i = size_;
    try {
      for (; i < new_size; ++i) {
        new (ptrAt(i)) T();
      }
    } catch (...) {
      destroyRange(size_, i);
      throw;
    }
    size_ = new_size;
  }

  void clear() {
    destroyElements();
    size_ = 0;
  }

  void reserve(std::size_t new_capacity) {
    if (new_capacity <= capacity_) {
      return;
    }
    ensureCapacityFor(new_capacity);
  }

  std::size_t size() const noexcept { return size_; }
  std::size_t capacity() const noexcept { return capacity_; }
  bool empty() const noexcept { return size_ == 0; }

  T &operator[](std::size_t idx) noexcept { return *ptrAt(idx); }
  const T &operator[](std::size_t idx) const noexcept { return *ptrAt(idx); }

private:
  using BlockPtr = T *;

  void ensureCapacityFor(std::size_t required) {
    if (required <= capacity_) {
      return;
    }
    const std::size_t needed_blocks =
        (required + BlockSize - 1) / BlockSize;
    while (blocks_.size() < needed_blocks) {
      blocks_.pushBack(allocateBlock());
    }
    capacity_ = blocks_.size() * BlockSize;
  }

  BlockPtr allocateBlock() {
    return static_cast<BlockPtr>(::operator new(sizeof(T) * BlockSize));
  }

  void releaseBlocks() {
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      ::operator delete(blocks_[i]);
    }
    blocks_.clear();
    capacity_ = 0;
  }

  T *ptrAt(std::size_t idx) noexcept {
    const std::size_t block_index = idx / BlockSize;
    const std::size_t offset = idx % BlockSize;
    return blocks_[block_index] + offset;
  }

  const T *ptrAt(std::size_t idx) const noexcept {
    const std::size_t block_index = idx / BlockSize;
    const std::size_t offset = idx % BlockSize;
    return blocks_[block_index] + offset;
  }

  void destroyRange(std::size_t begin, std::size_t end) {
    for (std::size_t i = begin; i < end; ++i) {
      ptrAt(i)->~T();
    }
  }

  void destroyElements() { destroyRange(0, size_); }

  ::orteaf::internal::base::HeapVector<BlockPtr> blocks_{};
  std::size_t size_{0};
  std::size_t capacity_{0};
};

} // namespace orteaf::internal::base
