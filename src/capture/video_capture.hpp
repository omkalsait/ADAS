#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include <opencv2/videoio.hpp>

#include "include/fcw/bounded_queue.hpp"
#include "include/fcw/frame_pool.hpp"
#include "include/fcw/pipeline_messages.hpp"

namespace fcw {

struct CaptureOptions {
  std::string video_path;
  double fps = 0.0;
  bool loop = false;
  double duration_s = 30.0;
};

using DropRecorder = std::function<void(const char*, uint64_t, int64_t)>;

class VideoCaptureStage {
 public:
  bool open(const CaptureOptions& options, std::string* error);
  int width() const noexcept;
  int height() const noexcept;
  double effective_fps() const noexcept;

  void run(FramePool* pool, BoundedQueue<Frame>* output,
           std::atomic<bool>* keep_running, const DropRecorder& record_drop);

 private:
  CaptureOptions options_;
  cv::VideoCapture capture_;
  int width_ = 0;
  int height_ = 0;
  double effective_fps_ = 0.0;
};

}  // namespace fcw
