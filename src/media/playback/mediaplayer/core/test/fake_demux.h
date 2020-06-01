// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_MEDIA_PLAYBACK_MEDIAPLAYER_CORE_TEST_FAKE_DEMUX_H_
#define SRC_MEDIA_PLAYBACK_MEDIAPLAYER_CORE_TEST_FAKE_DEMUX_H_

#include <lib/fit/function.h>

#include "src/media/playback/mediaplayer/demux/demux.h"

namespace media_player {
namespace test {

class FakeDemux : public Demux {
 public:
  static std::shared_ptr<FakeDemux> Create();

  FakeDemux();

  ~FakeDemux() override {}

  const char* label() const override { return "FakeDemux"; }

  // Demux implementation.
  void ConfigureConnectors() override {
    for (size_t output_index = 0; output_index < streams_.size(); ++output_index) {
      ConfigureOutputToProvideLocalMemory(0,        // max_aggregate_payload_size
                                          1,        // max_payload_size
                                          0,        // map_flags
                                          nullptr,  // video_constraints
                                          output_index);
    }
  }

  void FlushOutput(size_t output_index, fit::closure callback) override { callback(); }

  void RequestOutputPacket() override {}

  void SetStatusCallback(StatusCallback callback) override {
    status_callback_ = std::move(callback);
  }

  void SetCacheOptions(zx_duration_t lead, zx_duration_t backtrack) override {}

  void WhenInitialized(fit::function<void(zx_status_t)> callback) override { callback(ZX_OK); }

  const std::vector<std::unique_ptr<DemuxStream>>& streams() const override { return streams_; }

  void Seek(int64_t position, SeekCallback callback) override {}

 private:
  class DemuxStreamImpl : public DemuxStream {
   public:
    DemuxStreamImpl(size_t index, std::unique_ptr<StreamType> stream_type,
                    media::TimelineRate pts_rate)
        : index_(index), stream_type_(std::move(stream_type)), pts_rate_(pts_rate) {}

    ~DemuxStreamImpl() override {}

    size_t index() const override { return index_; }

    std::unique_ptr<StreamType> stream_type() const override { return stream_type_->Clone(); }

    media::TimelineRate pts_rate() const override { return pts_rate_; }

   private:
    size_t index_;
    std::unique_ptr<StreamType> stream_type_;
    media::TimelineRate pts_rate_;
  };

  StatusCallback status_callback_;
  std::vector<std::unique_ptr<DemuxStream>> streams_;
};

}  // namespace test
}  // namespace media_player

#endif  // SRC_MEDIA_PLAYBACK_MEDIAPLAYER_CORE_TEST_FAKE_DEMUX_H_
