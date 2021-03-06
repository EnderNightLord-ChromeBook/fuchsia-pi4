// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_MEDIA_AUDIO_AUDIO_CORE_TESTING_FAKE_AUDIO_CLOCK_H_
#define SRC_MEDIA_AUDIO_AUDIO_CORE_TESTING_FAKE_AUDIO_CLOCK_H_

#include "src/media/audio/audio_core/testing/fake_clock_manager.h"

namespace media::audio::testing {

class FakeAudioClock : public AudioClock {
 public:
  static FakeAudioClock ClientAdjustable(std::shared_ptr<FakeClockManager> manager,
                                         zx::clock clock) {
    return FakeAudioClock(manager, std::move(clock), Source::Client, true);
  }

  static FakeAudioClock ClientFixed(std::shared_ptr<FakeClockManager> manager, zx::clock clock) {
    return FakeAudioClock(manager, std::move(clock), Source::Client, false);
  }

  static FakeAudioClock DeviceAdjustable(std::shared_ptr<FakeClockManager> manager, zx::clock clock,
                                         uint32_t domain) {
    return FakeAudioClock(manager, std::move(clock), Source::Device, true, domain);
  }

  static FakeAudioClock DeviceFixed(std::shared_ptr<FakeClockManager> manager, zx::clock clock,
                                    uint32_t domain) {
    return FakeAudioClock(manager, std::move(clock), Source::Device, false, domain);
  }

  TimelineFunction ref_clock_to_clock_mono() const override {
    return manager_->ref_to_mono_time_transform(clock_id_);
  }

  zx::time ReferenceTimeFromMonotonicTime(zx::time mono_time) const override {
    return zx::time{manager_->ref_to_mono_time_transform(clock_id_).ApplyInverse(mono_time.get())};
  }

  zx::time MonotonicTimeFromReferenceTime(zx::time ref_time) const override {
    return zx::time{manager_->ref_to_mono_time_transform(clock_id_).Apply(ref_time.get())};
  }

  zx::time Read() const override { return ReferenceTimeFromMonotonicTime(manager_->mono_time()); }

  void UpdateClockRate(int32_t rate_adjust_ppm) override {
    // Simulate zx_clock_update, which ignores out-of-range rates.
    if (rate_adjust_ppm < ZX_CLOCK_UPDATE_MIN_RATE_ADJUST ||
        rate_adjust_ppm > ZX_CLOCK_UPDATE_MAX_RATE_ADJUST) {
      FX_LOGS(WARNING) << "FakeAudioClock::UpdateClockRate rate_adjust_ppm out of bounds";
      return;
    }
    manager_->UpdateClockRate(clock_id_, rate_adjust_ppm);
  }

 private:
  FakeAudioClock(std::shared_ptr<FakeClockManager> manager, zx::clock clock, Source source,
                 bool adjustable, uint32_t domain = kInvalidDomain)
      : AudioClock(std::move(clock), source, adjustable, domain),
        manager_(manager),
        clock_id_(audio::clock::GetKoid(DuplicateClock().take_value())) {}

  std::shared_ptr<FakeClockManager> manager_;
  zx_koid_t clock_id_;
};

}  // namespace media::audio::testing

#endif  // SRC_MEDIA_AUDIO_AUDIO_CORE_TESTING_FAKE_AUDIO_CLOCK_H_
