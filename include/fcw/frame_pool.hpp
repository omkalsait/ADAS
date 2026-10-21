#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <opencv2/core/mat.hpp>

namespace fcw {

// Reuses fixed image buffers. Lease copies only adjust an intrusive reference
// count; image storage is allocated once during pool construction.
class FramePool {
 private:
  struct Slot {
    cv::Mat image;
    size_t references = 0;
  };

 public:
  class Lease {
   public:
    Lease() = default;
    Lease(const Lease& other) : owner_(other.owner_), index_(other.index_) {
      if (owner_ != nullptr) owner_->retain(index_);
    }
    Lease& operator=(const Lease& other) {
      if (this != &other) {
        reset();
        owner_ = other.owner_;
        index_ = other.index_;
        if (owner_ != nullptr) owner_->retain(index_);
      }
      return *this;
    }
    Lease(Lease&& other) noexcept
        : owner_(other.owner_), index_(other.index_) {
      other.owner_ = nullptr;
    }
    Lease& operator=(Lease&& other) noexcept {
      if (this != &other) {
        reset();
        owner_ = other.owner_;
        index_ = other.index_;
        other.owner_ = nullptr;
      }
      return *this;
    }
    ~Lease() { reset(); }

    explicit operator bool() const noexcept { return owner_ != nullptr; }
    cv::Mat& image() { return owner_->slots_[index_].image; }
    const cv::Mat& image() const { return owner_->slots_[index_].image; }
    void reset() {
      if (owner_ != nullptr) {
        FramePool* pool = owner_;
        owner_ = nullptr;
        pool->release(index_);
      }
    }

   private:
    friend class FramePool;
    Lease(FramePool* owner, size_t index) : owner_(owner), index_(index) {}
    FramePool* owner_ = nullptr;
    size_t index_ = 0;
  };

  FramePool(size_t count, int width, int height, int type)
      : count_(count), slots_(new Slot[count]) {
    if (count == 0 || width <= 0 || height <= 0) {
      throw std::invalid_argument("FramePool dimensions and count must be positive");
    }
    free_.reserve(count);
    for (size_t i = 0; i < count_; ++i) {
      slots_[i].image.create(height, width, type);
      free_.push_back(count_ - i - 1);
    }
  }

  FramePool(const FramePool&) = delete;
  FramePool& operator=(const FramePool&) = delete;

  Lease acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    available_.wait(lock, [this] { return !free_.empty(); });
    const size_t index = free_.back();
    free_.pop_back();
    slots_[index].references = 1;
    return Lease(this, index);
  }

 private:
  void retain(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++slots_[index].references;
  }
  void release(size_t index) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (--slots_[index].references == 0) free_.push_back(index);
    }
    available_.notify_one();
  }

  const size_t count_;
  std::unique_ptr<Slot[]> slots_;
  std::vector<size_t> free_;
  std::mutex mutex_;
  std::condition_variable available_;
};

}  // namespace fcw
