#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fcw {

template <typename T>
struct PushResult {
  bool accepted = false;
  std::optional<T> dropped_value;

  bool dropped() const noexcept { return dropped_value.has_value(); }
  // Preserves convenient `if (queue.push(value))` drop detection.
  operator bool() const noexcept { return dropped(); }
};

// Fixed-capacity ring storage: construction allocates slots once; push/pop do not
// grow a container. When full, push evicts and returns the oldest item.
template <typename T>
class BoundedQueue {
 public:
  explicit BoundedQueue(size_t capacity)
      : slots_(capacity), capacity_(capacity) {
    if (capacity_ == 0) {
      throw std::invalid_argument("BoundedQueue capacity must be positive");
    }
  }

  BoundedQueue(const BoundedQueue&) = delete;
  BoundedQueue& operator=(const BoundedQueue&) = delete;

  PushResult<T> push(T value) {
    std::lock_guard<std::mutex> lock(mutex_);
    PushResult<T> result;
    if (closed_) {
      return result;
    }
    if (size_ == capacity_) {
      result.dropped_value.emplace(std::move(*slots_[head_]));
      slots_[head_].reset();
      head_ = (head_ + 1) % capacity_;
      --size_;
      ++drops_;
    }
    const size_t tail = (head_ + size_) % capacity_;
    slots_[tail].emplace(std::move(value));
    ++size_;
    result.accepted = true;
    ready_.notify_one();
    return result;
  }

  std::optional<T> pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    ready_.wait(lock, [this] { return size_ != 0 || closed_; });
    if (size_ == 0) {
      return std::nullopt;
    }
    std::optional<T> result(std::move(*slots_[head_]));
    slots_[head_].reset();
    head_ = (head_ + 1) % capacity_;
    --size_;
    return result;
  }

  void close() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
    }
    ready_.notify_all();
  }

  uint64_t drops() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return drops_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable ready_;
  std::vector<std::optional<T>> slots_;
  const size_t capacity_;
  size_t head_ = 0;
  size_t size_ = 0;
  uint64_t drops_ = 0;
  bool closed_ = false;
};

}  // namespace fcw
