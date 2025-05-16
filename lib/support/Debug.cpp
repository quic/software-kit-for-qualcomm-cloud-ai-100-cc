// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Debug.h"
#include "llvm/ADT/StringRef.h"

using namespace llvm;

void qaic::initDebugging(bool debugFlag) {
  if (debugFlag) {
    DebugFlag = true;
    return;
  }

  if (const char *env = std::getenv("QAIC_DEBUG")) {
    if (env && (StringRef(env) == "1")) {
      DebugFlag = true;
    }
  }
}
