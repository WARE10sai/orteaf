#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "orteaf/internal/base/heap_vector.h"

namespace orteaf::internal::base {

/**
 * @brief Segmented vector with stable element addresses on grow.
 *
 * BlockVector stores elements in fixed-size blocks allocated independently,
 * so growing the container never relocates existing elements. This is useful
 * when external code holds raw pointers into the storage.
 *
 * Design notes:
 * - Storage is contiguous within each block, not globally contiguous.
 * - Iteration is supported via a random-access iterator that walks by index.
 * - Growing never moves existing elements, but may allocate new blocks.
 */
template <typename T, std::size_t BlockSize = 64>
class BlockVector {
public:
  static_assert(BlockSize > 0, "BlockSize must be > 0");

  class iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    iterator() = default;
    reference operator*() const { return (*owner_)[index_]; }
    pointer operator->() const { return &(*owner_)[index_]; }

    iterator &operator++() {
      ++index_;
      return *this;
    }
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    iterator &operator--() {
      --index_;
      return *this;
    }
    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    iterator &operator+=(difference_type n) {
      index_ += static_cast<std::size_t>(n);
      return *this;
    }
    iterator &operator-=(difference_type n) {
      index_ -= static_cast<std::size_t>(n);
      return *this;
    }
    iterator operator+(difference_type n) const {
      iterator tmp = *this;
      return tmp += n;
    }
    iterator operator-(difference_type n) const {
      iterator tmp = *this;
      return tmp -= n;
    }
    difference_type operator-(const iterator &other) const {
      return static_cast<difference_type>(index_) -
             static_cast<difference_type>(other.index_);
    }

    bool operator==(const iterator &other) const {
      return owner_ == other.owner_ && index_ == other.index_;
    }
    bool operator!=(const iterator &other) const { return !(*this == other); }
    bool operator<(const iterator &other) const { return index_ < other.index_; }
    bool operator<=(const iterator &other) const {
      return index_ <= other.index_;
    }
    bool operator>(const iterator &other) const { return index_ > other.index_; }
    bool operator>=(const iterator &other) const {
      return index_ >= other.index_;
    }

  private:
    friend class BlockVector;
    iterator(BlockVector *owner, std::size_t index) : owner_(owner), index_(index) {}

    BlockVector *owner_{nullptr};
    std::size_t index_{0};
  };

  class const_iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = const T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;

    const_iterator() = default;
    reference operator*() const { return (*owner_)[index_]; }
    pointer operator->() const { return &(*owner_)[index_]; }

    const_iterator &operator++() {
      ++index_;
      return *this;
    }
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    const_iterator &operator--() {
      --index_;
      return *this;
    }
    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }

    const_iterator &operator+=(difference_type n) {
      index_ += static_cast<std::size_t>(n);
      return *this;
    }
    const_iterator &operator-=(difference_type n) {
      index_ -= static_cast<std::size_t>(n);
      return *this;
    }
    const_iterator operator+(difference_type n) const {
      const_iterator tmp = *this;
      return tmp += n;
    }
    const_iterator operator-(difference_type n) const {
      const_iterator tmp = *this;
      return tmp -= n;
    }
    difference_type operator-(const const_iterator &other) const {
      return static_cast<difference_type>(index_) -
             static_cast<difference_type>(other.index_);
    }

    bool operator==(const const_iterator &other) const {
      return owner_ == other.owner_ && index_ == other.index_;
    }
    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }
    bool operator<(const const_iterator &other) const {
      return index_ < other.index_;
    }
    bool operator<=(const const_iterator &other) const {
      return index_ <= other.index_;
    }
    bool operator>(const const_iterator &other) const {
      return index_ > other.index_;
    }
    bool operator>=(const const_iterator &other) const {
      return index_ >= other.index_;
    }

  private:
    friend class BlockVector;
    const_iterator(const BlockVector *owner, std::size_t index)
        : owner_(owner), index_(index) {}

    const BlockVector *owner_{nullptr};
    std::size_t index_{0};
  };

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

  /// @brief Resize to new_size with default-constructed elements.
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

  /// @brief Append a copy of value (copy-constructible only).
  void pushBack(const T &value)
    requires std::is_copy_constructible_v<T>
  {
    emplaceBack(value);
  }
  void pushBack(const T &)
    requires (!std::is_copy_constructible_v<T>)
  = delete;
  /// @brief Append a moved value.
  void pushBack(T &&value) { emplaceBack(static_cast<T &&>(value)); }

  /// @brief Append an element constructed in-place.
  template <typename... Args>
  T &emplaceBack(Args &&...args) {
    ensureCapacityFor(size_ + 1);
    T *slot = ptrAt(size_);
    new (slot) T(static_cast<Args &&>(args)...);
    ++size_;
    return *slot;
  }

  /// @brief Remove the last element if present.
  void popBack() {
    if (size_ == 0) {
      return;
    }
    const std::size_t last = size_ - 1;
    ptrAt(last)->~T();
    size_ = last;
  }

  /// @brief Destroy all elements without releasing blocks.
  void clear() {
    destroyElements();
    size_ = 0;
  }

  /// @brief Reserve capacity for at least new_capacity elements.
  void reserve(std::size_t new_capacity) {
    if (new_capacity <= capacity_) {
      return;
    }
    ensureCapacityFor(new_capacity);
  }

  /// @brief Returns the number of elements.
  std::size_t size() const noexcept { return size_; }
  /// @brief Returns the total capacity across blocks.
  std::size_t capacity() const noexcept { return capacity_; }
  /// @brief Returns true if empty.
  bool empty() const noexcept { return size_ == 0; }

  /// @brief Unchecked element access.
  T &operator[](std::size_t idx) noexcept { return *ptrAt(idx); }
  /// @brief Unchecked element access.
  const T &operator[](std::size_t idx) const noexcept { return *ptrAt(idx); }

  /// @brief Bounds-checked element access.
  T &at(std::size_t idx) {
    if (idx >= size_) {
      throw std::out_of_range("BlockVector::at");
    }
    return (*this)[idx];
  }
  /// @brief Bounds-checked element access.
  const T &at(std::size_t idx) const {
    if (idx >= size_) {
      throw std::out_of_range("BlockVector::at");
    }
    return (*this)[idx];
  }

  /// @brief Access the first element.
  T &front() noexcept { return (*this)[0]; }
  /// @brief Access the first element.
  const T &front() const noexcept { return (*this)[0]; }
  /// @brief Access the last element.
  T &back() noexcept { return (*this)[size_ - 1]; }
  /// @brief Access the last element.
  const T &back() const noexcept { return (*this)[size_ - 1]; }

  /// @brief Returns iterator to the first element.
  iterator begin() noexcept { return iterator(this, 0); }
  /// @brief Returns iterator to one past the last element.
  iterator end() noexcept { return iterator(this, size_); }
  /// @brief Returns const iterator to the first element.
  const_iterator begin() const noexcept { return const_iterator(this, 0); }
  /// @brief Returns const iterator to one past the last element.
  const_iterator end() const noexcept { return const_iterator(this, size_); }
  /// @brief Returns const iterator to the first element.
  const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
  /// @brief Returns const iterator to one past the last element.
  const_iterator cend() const noexcept { return const_iterator(this, size_); }

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
