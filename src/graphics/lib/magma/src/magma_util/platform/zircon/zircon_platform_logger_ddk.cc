// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdarg.h>
#include <stdio.h>

#include <ddk/debug.h>

#include "platform_logger.h"

namespace magma {

bool PlatformLogger::IsInitialized() { return true; }

bool PlatformLogger::Initialize(std::unique_ptr<PlatformHandle> handle) { return true; }

void PlatformLogger::LogVa(LogLevel level, const char* file, int line, const char* fmt,
                           va_list args) {
  // TODO: Propogate file and line via caller.
  switch (level) {
    case PlatformLogger::LOG_ERROR:
      zxlogvf(ERROR, file, line, fmt, args);
      return;
    case PlatformLogger::LOG_WARNING:
      zxlogvf(WARNING, file, line, fmt, args);
      return;
    case PlatformLogger::LOG_INFO:
      zxlogvf(INFO, file, line, fmt, args);
      return;
  }
}

}  // namespace magma
