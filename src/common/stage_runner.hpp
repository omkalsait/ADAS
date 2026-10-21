#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <type_traits>
#include <utility>

#include "include/fcw/bounded_queue.hpp"
#include "include/fcw/clock.hpp"
#include "include/fcw/pipeline_messages.hpp"

namespace fcw {

// One runner owns one stage thread. Processor returns false to mark a message
// as erroneous while still forwarding it for end-to-end diagnostics.
template <typename Input, typename Output, typename Processor, typename OnDrop>
class StageRunner {
 public:
  StageRunner(const char* queue_name, Stage stage, BoundedQueue<Input>* input,
              BoundedQueue<Output>* output, Processor processor,
              OnDrop on_drop)
      : queue_name_(queue_name),
        stage_(stage),
        input_(input),
        output_(output),
        processor_(std::move(processor)),
        on_drop_(std::move(on_drop)) {}

  StageRunner(const StageRunner&) = delete;
  StageRunner& operator=(const StageRunner&) = delete;
  ~StageRunner() { join(); }

  void start() { thread_ = std::thread([this] { run(); }); }
  void join() {
    if (thread_.joinable()) thread_.join();
  }

 private:
  void run() {
    while (auto input = input_->pop()) {
      Output output;
      output.header = input->header;
      output.timestamps = input->timestamps;
      const int64_t t_in = now_ns();
      if (!processor_(*input, &output)) output.header.status = Status::kError;
      const int64_t t_out = now_ns();
      output.header.t_in_ns = t_in;
      output.header.t_out_ns = t_out;
      output.timestamps.at(stage_).t_in_ns = t_in;
      output.timestamps.at(stage_).t_out_ns = t_out;
      PushResult<Output> result = output_->push(std::move(output));
      if (!result.accepted) break;
      if (result.dropped_value) {
        on_drop_(queue_name_, result.dropped_value->header.frame_id, now_ns());
      }
    }
    output_->close();
  }

  const char* queue_name_;
  Stage stage_;
  BoundedQueue<Input>* input_;
  BoundedQueue<Output>* output_;
  Processor processor_;
  OnDrop on_drop_;
  std::thread thread_;
};

}  // namespace fcw
