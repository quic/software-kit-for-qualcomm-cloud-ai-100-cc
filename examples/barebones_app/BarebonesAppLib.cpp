// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "BarebonesAppLib.h"

void run(AICExecContext *qctx, uint8_t tid, uint32_t stid) {
  NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_DEBUG, "activate() for thread %d.",
         tid);
}
