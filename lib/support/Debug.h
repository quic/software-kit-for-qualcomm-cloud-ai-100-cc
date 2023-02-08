// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_SUPPORT_DEBUG_H_
#define _QAIC_SUPPORT_DEBUG_H_

#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace qaic {

/**
 * @brief Initialize debugging either by setting the given flag or
 * loading the QAIC_DEBUG environment variable.
 */
void initDebugging(bool debugFlag = false);

#define QAIC_DEBUG(X)                                                          \
  do {                                                                         \
    LLVM_DEBUG(llvm::dbgs() << DEBUG_TYPE << ": ");                            \
    LLVM_DEBUG(X);                                                             \
  } while (false)

#define QAIC_DEBUG_STREAM(X) QAIC_DEBUG(llvm::dbgs() << X)
} // namespace qaic

#endif
