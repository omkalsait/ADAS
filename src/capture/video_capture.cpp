#include "src/capture/video_capture.hpp"

#include <chrono>
#include <cmath>
#include <thread>
#include <utility>

#include "include/fcw/clock.hpp"

namespace fcw {
namespace {
constexpr double kUseSourceFps = 0.0;
}

bool VideoCaptureStage::open(const CaptureOptions& options, std::string* error) {
  options_ = options;
  if (!capture_.open(options_.video_path)) {
    if (error != nullptr) *error = "could not open video: " + options_.video_path;
    return false;
  }
  width_ = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
  height_ = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
  const double source_fps = capture_.get(cv::CAP_PROP_FPS);
  effective_fps_ = options_.fps == kUseSourceFps ? source_fps : options_.fps;
  if (width_ <= 0 || height_ <= 0 || !std::isfinite(effective_fps_) ||
      effective_fps_ <= 0.0 || !std::isfinite(options_.duration_s) ||
      options_.duration_s <= 0.0) {
    if (error != nullptr) {
      *error = "video dimensions, effective fps, and duration must be positive";
    }
    capture_.release();
    return false;
  }
  return true;
}

int VideoCaptureStage::width() const noexcept { return width_; }
int VideoCaptureStage::height() const noexcept { return height_; }
double VideoCaptureStage::effective_fps() const noexcept { return effective_fps_; }

void VideoCaptureStage::run(FramePool* pool, BoundedQueue<Frame>* output,
                            std::atomic<bool>* keep_running,
                            const DropRecorder& record_drop) {
  using Clock = std::chrono::steady_clock;
  const auto period = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(1.0 / effective_fps_));
  const auto run_start = Clock::now();
  auto next_release = run_start;
  uint64_t frame_id = 0;

  while (keep_running->load()) {
    const auto read_start = Clock::now();
    Frame frame;
    frame.image = pool->acquire();
    if (!keep_running->load()) break;
    if (!capture_.read(frame.image.image())) {
      if (!options_.loop) break;
      capture_.set(cv::CAP_PROP_POS_FRAMES, 0.0);
      if (!capture_.read(frame.image.image())) break;
    }

    // Keep source-read time distinct from pacing. The capture stage's exit
    // timestamp includes pacing so pipeline latency includes deliberate waits.
    const int64_t captured_ns = now_ns();
    next_release += period;
    std::this_thread::sleep_until(next_release);
    const int64_t released_ns = now_ns();
    frame.header.frame_id = frame_id++;
    frame.header.t_capture_ns = captured_ns;
    frame.header.t_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               read_start.time_since_epoch())
                               .count();
    frame.header.t_out_ns = released_ns;
    frame.timestamps.at(Stage::kCapture) =
        {frame.header.t_in_ns, frame.header.t_out_ns};

    PushResult<Frame> result = output->push(std::move(frame));
    if (!result.accepted) break;
    if (result.dropped_value && record_drop) {
      record_drop("capture_preprocess", result.dropped_value->header.frame_id,
                  now_ns());
    }

    if (Clock::now() - run_start >=
        std::chrono::duration<double>(options_.duration_s)) {
      break;
    }
  }
  output->close();
}

}  // namespace fcw
