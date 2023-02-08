// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DriverContext.h"
#include "Driver.h"
#include "program/ProgramConfig.h"

using namespace llvm;
using namespace qaic;

#define DRIVER_CONTEXT_REPORT_ERROR(X)                                         \
  do {                                                                         \
    llvm::errs() << "error: " << X;                                            \
  } while (false)

bool DriverContext::cleanupIntermediateFiles() const {
  bool removedAllOk = true;
  for (auto &f : intermediateFiles_) {
    auto ec = sys::fs::remove(f);
    if (ec) {
      DRIVER_CONTEXT_REPORT_ERROR("failed to remove temorary file " << f
                                                                    << "\n");
      removedAllOk = false;
    }
  }
  return removedAllOk;
}

bool DriverContext::run() {
  // Run the actions
  bool encounteredError = false;
  for (auto &Action : actionsToRun_) {
    if (!Action->run(*this)) {
      encounteredError = true;
      break;
    }
  }

  if (!encounteredError) {
    encounteredError |= !(cleanupIntermediateFiles());
  }

  return !encounteredError;
}
