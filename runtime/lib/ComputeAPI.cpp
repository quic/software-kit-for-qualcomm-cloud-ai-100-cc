// Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ComputeAPI.h"

#include "NSPContext.h"
#include "SerializedProgramDesc.h"
#include "libdev/os-inlines.h"
#include "libdev/os.h"

namespace qaic {

bool isBufferValid(int buffNum) {
  const BufferDesc_t *buff = &_progBuffers[buffNum];
  const uint16_t waitDBNum = buff->waitDBNum;
  const uint32_t *dbs = (uint32_t *)getNSPContext()->baseL2TCM;
  return (buff->waitDBVal == dbs[waitDBNum]);
}

void clearBufferValid(int buffNum) {
  const BufferDesc_t *buff = &_progBuffers[buffNum];
  const uint16_t waitDBNum = buff->waitDBNum;
  uint32_t *dbs = (uint32_t *)getNSPContext()->baseL2TCM;
  os_doorbell_local_write4b(&dbs[waitDBNum], 0);
}

void waitForBuffer(int buffNum, uint32_t waitDBVal, bool clear) {
  const BufferDesc_t *buff = &_progBuffers[buffNum];
  uint16_t waitDBNum = buff->waitDBNum;
  uint32_t *dbs = (uint32_t *)getNSPContext()->baseL2TCM;
  os_doorbell_wait_eq((nsp_doorbell_t)&dbs[waitDBNum], waitDBVal,
                      /*doTimeoutCheck*/ false,
                      /*threadId*/ 0);
  if (clear) {
    os_doorbell_local_write4b(&dbs[waitDBNum], 0);
  }
}

void waitForBuffer(int buffNum, bool clear) {
  const BufferDesc_t *buff = &_progBuffers[buffNum];
  waitForBuffer(buffNum, buff->waitDBVal, clear);
}

void waitForAllInputsReady(bool clear) {
  const uint16_t numBuffs = _progDesc->numInputBuffs;
  const uint16_t numInputs = _progDesc->numInputBuffs;
  uint16_t numInputsReady = 0;

  for (int buffNum = 0; (numInputsReady < numInputs) && (buffNum < numBuffs);
       buffNum++) {
    const BufferDesc_t *buff = &_progBuffers[buffNum];
    if (buff->usage == USAGE_INPUT) {
      waitForBuffer(buffNum, clear);
      numInputsReady++;
    }
  }
  if (numInputsReady != numInputs) {
    ERR_FATAL(getNSPContext()->errFuncPtr,
              "Only found %d ready inputs, but expected to find %d",
              numInputsReady, numInputs, 0);
    __builtin_unreachable();
  }
}

void waitForAllOutputsReady(bool clear) {
  const uint16_t numBuffs = _progDesc->numBuffs;
  const uint16_t numOutputs = _progDesc->numOutputBuffs;
  uint16_t numOutputsReady = 0;

  for (int buffNum = 0; (numOutputsReady < numOutputs) && (buffNum < numBuffs);
       buffNum++) {
    const BufferDesc_t *buff = &_progBuffers[buffNum];
    if (buff->usage == USAGE_OUTPUT) {
      waitForBuffer(buffNum, clear);
      numOutputsReady++;
    }
  }
  if (numOutputsReady != numOutputs) {
    ERR_FATAL(getNSPContext()->errFuncPtr,
              "Only found %d ready outputs, but expected to find %d",
              numOutputsReady, numOutputs, 0);
    __builtin_unreachable();
  }
}

void readyForAllInputs(bool waitForArrival, bool clear) {
  // Input from host
  CoreInfo *ctx = getNSPContext();
  if ((0x1 << ctx->virtualNSPId) & _progDesc->hasInputsMask) {
    uint16_t inputSem = _progDesc->inputSem;

    hostsem_t semAddr =
        (hostsem_t *)getNSPContext()->semInfo[inputSem].semAddress;
    auto fwSemIdx = getNSPContext()->semInfo[inputSem].semNum;

    // Clear the input doorbells before doing the host increment
    const uint16_t numBuffs = _progDesc->numBuffs;
    const uint16_t numInputs = _progDesc->numInputBuffs;
    uint32_t *dbs = (uint32_t *)getNSPContext()->baseL2TCM;
    uint16_t numInputsCleared = 0;
    for (int buffNum = 0;
         (numInputsCleared < numInputs) && (buffNum < numBuffs); buffNum++) {
      const BufferDesc_t *buff = &_progBuffers[buffNum];
      if (buff->usage == USAGE_INPUT) {
        uint16_t waitDBNum = buff->waitDBNum;
        os_doorbell_local_write4b(&dbs[waitDBNum], 0);
        numInputsCleared++;
      }
    }

    // Make sure all reads are done before doing semaphore
    os_release_allthreads(&ctx->inputSemaphoreReleaseLoc);
    os_load_acquire(&ctx->inputSemaphoreReleaseLoc);

    os_hostsem_inc(semAddr, static_cast<uint32_t>(fwSemIdx));
    if (waitForArrival) {
      waitForAllInputsReady(clear);
    }
  } else {
    NN_LOG(ctx->logFuncPtr, NNC_LOG_MASK_WARN,
           "NSP %d called readyForAllInputs, but doesn't have inputs",
           ctx->virtualNSPId);
  }
}

void sendAllOutputs(bool waitForArrival, bool clear) {
  // Output to host
  CoreInfo *ctx = getNSPContext();
  if ((0x1 << ctx->virtualNSPId) & _progDesc->hasInputsMask) {
    uint16_t outputSem = _progDesc->outputSem;

    hostsem_t semAddr =
        (hostsem_t *)getNSPContext()->semInfo[outputSem].semAddress;
    int fwSemIdx = getNSPContext()->semInfo[outputSem].semNum;

    // Clear the output doorbells before doing the host decrement
    const uint16_t numBuffs = _progDesc->numBuffs;
    const uint16_t numOutputs = _progDesc->numOutputBuffs;
    uint32_t *dbs = (uint32_t *)getNSPContext()->baseL2TCM;
    uint16_t numOutputsCleared = 0;
    for (int buffNum = 0;
         (numOutputsCleared < numOutputs) && (buffNum < numBuffs); buffNum++) {
      const BufferDesc_t *buff = &_progBuffers[buffNum];
      if (buff->usage == USAGE_OUTPUT) {
        uint16_t waitDBNum = buff->waitDBNum;
        os_doorbell_local_write4b(&dbs[waitDBNum], 0);
        numOutputsCleared++;
      }
    }

    os_global_memsync();
    os_hostsem_dec(semAddr, fwSemIdx);
    if (waitForArrival) {
      waitForAllOutputsReady(clear);
    }
  } else {
    NN_LOG(ctx->logFuncPtr, NNC_LOG_MASK_WARN,
           "NSP %d called sendAllOutputs, but doesn't have outputs",
           ctx->virtualNSPId);
  }
}

void logActivate(uint8_t virtualThreadId) {
  CoreInfo *ctx = getNSPContext();

  NN_LOG(ctx->logFuncPtr, NNC_LOG_MASK_INFO,
        "NN_ACTIVATE_THREAD:  NSP %d Thread %d",
        ctx->virtualNSPId, virtualThreadId);
}
void logDeactivate(uint8_t virtualThreadId) {
  CoreInfo *ctx = getNSPContext();

  NN_LOG(ctx->logFuncPtr, NNC_LOG_MASK_INFO,
        "NN_DEACTIVATE_THREAD:  NSP %d Thread %d",
        ctx->virtualNSPId, virtualThreadId);
}

void registerExitFunc(void (*exitFunc)()) {
  getNSPContext()->exitThread = exitFunc;
}
} // namespace qaic
