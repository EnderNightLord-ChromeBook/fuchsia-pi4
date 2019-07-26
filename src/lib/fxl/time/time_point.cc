// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/lib/fxl/time/time_point.h"

#include "src/lib/fxl/build_config.h"

#if defined(OS_MACOSX) || defined(OS_IOS)
#include <mach/kern_return.h>
#include <mach/mach_time.h>
#elif defined(OS_FUCHSIA)
#include <zircon/syscalls.h>
#else
#include <time.h>
#endif  // defined(OS_MACOSX) || defined(OS_IOS)

#include "src/lib/fxl/logging.h"

namespace fxl {

// Mac OS X/iOS don't have a (useful) |clock_gettime()|.
// Note: Chromium's |base::TimeTicks::Now()| uses boot time (obtained via
// |sysctl()| with |CTL_KERN|/|KERN_BOOTTIME|). For our current purposes,
// monotonic time (which pauses during sleeps) is sufficient. TODO(vtl): If/when
// we use this for other purposes, maybe we should use boot time (maybe also on
// POSIX).
#if defined(OS_MACOSX) || defined(OS_IOS)

mach_timebase_info_data_t GetMachTimebaseInfo() {
  mach_timebase_info_data_t timebase_info = {};
  kern_return_t error = mach_timebase_info(&timebase_info);
  FXL_DCHECK(error == KERN_SUCCESS);
  return timebase_info;
}

// static
TimePoint TimePoint::Now() {
  static mach_timebase_info_data_t timebase_info = GetMachTimebaseInfo();
  return TimePoint(mach_absolute_time() * timebase_info.numer / timebase_info.denom);
}

#elif defined(OS_FUCHSIA)

// static
TimePoint TimePoint::Now() { return TimePoint(zx_clock_get_monotonic()); }

#else

// static
TimePoint TimePoint::Now() {
  struct timespec ts;
  int res = clock_gettime(CLOCK_MONOTONIC, &ts);
  FXL_DCHECK(res == 0);
  (void)res;
  return TimePoint::FromEpochDelta(TimeDelta::FromTimespec(ts));
}

#endif  // defined(OS_MACOSX) || defined(OS_IOS)

}  // namespace fxl
