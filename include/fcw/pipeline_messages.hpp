#pragma once

#include <array>
#include <cstdint>

#include <opencv2/core/mat.hpp>

#include "include/fcw/frame_pool.hpp"
#include "include/fcw/messages.hpp"

namespace fcw {

enum class Stage : size_t {
  kCapture = 0,
  kPreprocess,
  kInference,
  kTracking,
  kWarning,
  kSink,
  kCount
};

struct StageStamp {
  int64_t t_in_ns = 0;
  int64_t t_out_ns = 0;
};

struct StageTimestamps {
  std::array<StageStamp, static_cast<size_t>(Stage::kCount)> stage{};
  StageStamp& at(Stage which) { return stage[static_cast<size_t>(which)]; }
  const StageStamp& at(Stage which) const {
    return stage[static_cast<size_t>(which)];
  }
};

struct Frame {
  Header header;
  StageTimestamps timestamps;
  FramePool::Lease image;
};

struct Detection {
  float confidence = 0.0F;
  float left = 0.0F;
  float top = 0.0F;
  float right = 0.0F;
  float bottom = 0.0F;
};

struct Detections {
  static constexpr size_t kMaxDetections = 16;
  Header header;
  StageTimestamps timestamps;
  std::array<Detection, kMaxDetections> values{};
  size_t count = 0;
};

struct Tracks {
  Header header;
  StageTimestamps timestamps;
  std::array<Detection, Detections::kMaxDetections> values{};
  size_t count = 0;
};

enum class WarningState : uint8_t { kNone = 0, kVisual = 1, kAudible = 2 };

struct Warning {
  Header header;
  StageTimestamps timestamps;
  WarningState state = WarningState::kNone;
};

inline cv::Mat& image_of(Frame& frame) { return frame.image.image(); }
inline const cv::Mat& image_of(const Frame& frame) { return frame.image.image(); }

}  // namespace fcw
