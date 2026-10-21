#pragma once

#include <chrono>
#include <cstdint>

namespace fcw {

// All pipeline timestamps share this monotonic clock domain.
inline int64_t now_ns() noexcept {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

}  // namespace fcw
