#pragma once

#include <cstdint>

namespace fcw {

enum class Status : uint8_t { kOk = 0, kError = 1, kUnavailable = 2 };

struct Header {
  uint64_t frame_id = 0;
  int64_t t_capture_ns = 0;
  int64_t t_in_ns = 0;
  int64_t t_out_ns = 0;
  Status status = Status::kOk;
};

}  // namespace fcw
