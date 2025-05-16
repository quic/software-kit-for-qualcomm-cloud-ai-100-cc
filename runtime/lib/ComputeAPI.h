// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_COMPUTEAPI_H_
#define _QAIC_COMPUTEAPI_H_

#include "BufferDesc.h"
#include <stdint.h>

namespace qaic {
// I/O

/***
 * Returns true if the value for the buffer indicates it is valid
 * on the local NSP
 ***/
bool isBufferValid(int buffNum);

/***
 * Clears the valid indicator for the buffer on the local NSP
 ***/
void clearBufferValid(int buffNum);

/***
 * Blocks until all input buffers have been marked as ready,
 * and optionally clears the ready indications as they are received.
 *
 * Ready for input means data has been transferred into the buffer
 * by the host, and can be read by the NSP.
 ***/
void waitForAllInputsReady(bool clear);

/***
 * Blocks until all output buffers have been marked as ready,
 * and optionally clears the ready indications as they are received.
 *
 * Ready for output means that the host isn't currently in the process
 * of transferring the output buffer, so it is safe to write data into
 * it.
 ***/
void waitForAllOutputsReady(bool clear);

/***
 * Blocks until the specified buffer has been marked as ready,
 * and optionally clears the ready indication when it is received.
 *
 * Ready means it is safe for the NSP to read and write the buffer.
 ***/
void waitForBuffer(int buffNum, bool clear);

/***
 * Signals that this NSP is not reading from the input buffers,
 * so it is safe for the host to write into them.
 *
 * The host will start transferring data when each NSP has signalled
 * that it is ready.
 *
 * - waitForArrival can be set to cause this call to block until all
 * input transfers are complete
 * - clear can be set to cause waitForArrival to clear the ready
 * indications as it receives them
 ***/
void readyForAllInputs(bool waitForArrival, bool clear);

/***
 * Signals that this NSP is done writing to the output buffers,
 * so it is safe for the host to read from them.
 *
 * The host will start transferring data when each NSP has signalled
 * that it is ready.
 *
 * - waitForArrival can be set to cause this call to block until all
 * output transfers are complete
 * - clear can be set to cause waitForArrival to clear the ready
 * indications as it receives them
 ***/
void sendAllOutputs(bool waitForArrival, bool clear);

// SW Events
/***
 * Write a NN_ACTIVATE_THREAD NnSWEvent to the log
 *
 * - virtualThreadId is the tid that will appear in the log
 ***/
void logActivate(uint8_t virtualThreadId);

/***
 * Write a NN_DEACTIVATE_THREAD NnSWEvent to the log
 *
 * - virtualThreadId is the tid that will appear in the log
 ***/
void logDeactivate(uint8_t virtualThreadId);

/***
 * Register a function to call for thread exit
 *
 * Note: the function must end by calling the original
 *       qctx->exitThread function
 ***/
void registerExitFunc(void (*func)());

} // namespace qaic
#endif
