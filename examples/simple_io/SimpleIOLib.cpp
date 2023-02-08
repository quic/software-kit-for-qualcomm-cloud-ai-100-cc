// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SimpleIOLib.h"
#include "ComputeAPI.h"

using namespace qaic;

void run(AICExecContext *qctx, uint8_t tid, uint32_t stid) {
  if (tid == 0) {
    // Tell the host that it can send inputs -- only call from one thread per
    // NSP to avoid double-counting!
    readyForAllInputs(/*waitForArrival*/ false, /*clear*/ false);

    // Get the first input
    waitForBuffer(0, /*clear*/ true);
    char *buff0 = (char *)getBufferAddr(0);
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
           "Buffer 0 is ready and says: %s", buff0);

    // Get the second input
    waitForBuffer(1, /*clear*/ true);
    char *buff1 = (char *)getBufferAddr(1);
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
           "Buffer 1 is ready and says: %s", buff1);

    // Copy parts of the inputs to the output buffer
    waitForAllOutputsReady(/*clear*/ true);
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
           "Output buffer is ready for data");
    char *obuff = (char *)getBufferAddr(2);
    uint32_t outsize = getBufferSize(2);
    for (int i = 0; i < outsize; i++) {
      if ((i < outsize / 2) && (i < getBufferSize(0))) {
        obuff[i] = buff0[i];
      } else if (i < getBufferSize(1)) {
        obuff[i] = buff1[i];
      } else {
        obuff[i] = '\0';
      }
    }

    // Send the output
    sendAllOutputs(/*waitForArrival*/ true, /*clear*/ true);
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
           "Output buffers have been received");

  } else {
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
           "NSP %d thread %d has nothing to do!", qctx->virtualNSPId, tid);
  }
}
