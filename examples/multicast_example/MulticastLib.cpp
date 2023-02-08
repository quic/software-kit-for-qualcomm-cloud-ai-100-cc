// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "MulticastLib.h"
#include "ComputeAPI.h"

using namespace qaic;

// NSP 0 gets input buffers 0 and 1
// NSP 0 sends input buffer 0 to NSP 1 via internal buffer 1
// NSP 0 sends input buffer 1 to NSP 2 via internal buffer 2
// NSP 1 & 2 get process the buffers, and send back the result to internal
// buffer 0 and 3, respectively
// NSP 0 has internal buffer 0 & 3 mapped to output buffer 0
// NSP 0 sends output buffer to host

const char *digitStrings[] = {"zero",    "one",       "two",      "three",
                              "four",    "five",      "six",      "seven",
                              "eight",   "nine",      "ten",      "eleven",
                              "twelve",  "thirteen",  "fourteen", "fifteen",
                              "sixteen", "seventeen", "eighteen", "ERROR"};
const int maxDigitLength = 6;
const int maxDigitOutputLength = 10;
const int digitError = 19;
// Turns text digits into ints
int strToInt(const char *s) {
  for (int i = 0; i < 10; i++) {
    const char *digitStr = digitStrings[i];
    int j = 0;
    while (digitStr[j] == s[j] && j < maxDigitLength) {
      j++;
      if (digitStr[j] == '\0')
        return i;
    }
  }
  return digitError;
}

// Turns a digit into text
const char *digToStr(int i) {
  if (i >= 0 && i < digitError)
    return digitStrings[i];
  else
    return digitStrings[digitError];
}

// Copies a null-terminated string
int cpStr(char *dst, const char *src) {
  int i = 0;
  while (src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  return i;
}

int digitLen(const char *s) {
  int i = 0;
  while (s[i] != '\0' && i < maxDigitOutputLength) {
    i++;
  }
  return i;
}

// Reads A + B from the input buffer, calculates the result C, and writes
// A + B = C to the output buffer
void parseBuffer(AICExecContext *qctx, uint8_t tid) {
  int nspNum = qctx->virtualNSPId;

  // Wait for the buffer to arrive
  bool clear = true;
  int buffNum = internalBufferNum(nspNum);
  char *buff = (char *)getBufferAddr(buffNum);
  NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO, "Waiting for buffer %d", buffNum);
  waitForBuffer(buffNum, clear);
  NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO, "Buffer is ready and says: %s",
         buff);

  // Get the first digit
  const char *finalStr = nullptr;
  int pos = 0;
  uint32_t buffSize = getBufferSize(buffNum);
  int firstDigit = strToInt(&buff[pos]);
  pos += digitLen(digToStr(firstDigit));
  NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO, "First digit in \"%s\" is %d",
         buff, firstDigit);

  // Advance to the second digit
  if (firstDigit != digitError && buff[pos] == ' ' && buff[pos + 1] == '+' &&
      buff[pos + 2] == ' ') {
    pos += 3;
    int secondDigit = strToInt(&buff[pos]);
    pos += digitLen(digToStr(secondDigit));
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO, "Second digit in \"%s\" is %d",
           buff, secondDigit);

    // Create the result
    if (secondDigit != digitError) {
      int result = firstDigit + secondDigit;
      const char *resultStr = digToStr(result);
      int resultLen = digitLen(resultStr);
      cpStr(&buff[pos], " = ");
      pos += 3;
      cpStr(&buff[pos], digToStr(result));
      pos += resultLen;
      cpStr(&buff[pos], "\n");
      pos++;
      finalStr = buff;
    } else {
      // Second digit is error, so just send back error string
      pos = digitLen(digToStr(digitError));
      finalStr = digToStr(digitError);
    }
  } else {
    // First digit is error or parsing error, so just send back error string
    pos = digitLen(digToStr(digitError));
    finalStr = digToStr(digitError);
  }

  // Send the result back to a spot in interal buffer 0 (for NSP 1's output)
  // or internal buffer 3 (for NSP 2's output)
  int outBuffNum = internalBufferNum((nspNum == 1) ? 0 : 3);
  int outBuffSize = getBufferSize(outBuffNum);
  broadcastToBuffer(outBuffNum, 0, (pos < outBuffSize) ? pos : outBuffSize,
                    (int8_t *)finalStr, tid, /*waitForDone*/ false);
}

void run(AICExecContext *qctx, uint8_t tid, uint32_t stid) {
  if (qctx->virtualNSPId == 0) {
    if (tid == 0) {
      // NSP 0 Thread 0 - waits for input, and sends it on to NSP 1 & 2

      readyForAllInputs(/*waitForArrival*/ false, /*clear*/ false);

      // Since NSP 1 and 2 will send into our output buffer,
      // don't send data to them until our output buffer is ready to
      // be written
      waitForAllOutputsReady(/*clear*/ true);

      int buffNum;
      int buffSize;
      char *buff;

      // Clear the output buffer, since NSP 1 & 2 may only write partial data
      buffNum = outputBufferNum(0);
      buffSize = getBufferSize(buffNum);
      buff = (char *)getBufferAddr(buffNum);
      for (int i = 0; i < buffSize; i++) {
        buff[i] = '\0';
      }

      // Get the first input and send to internal buffer 1 (NSP 1) for
      // processing
      buffNum = inputBufferNum(0);
      waitForBuffer(buffNum, /*clear*/ false);
      buffSize = getBufferSize(buffNum);
      buff = (char *)getBufferAddr(buffNum);
      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "Input Buffer 0 is ready and says: %s", buff);
      int sendBuffNum = internalBufferNum(1);
      int sendBuffSize = getBufferSize(sendBuffNum);
      broadcastToBuffer(sendBuffNum, 0,
                        (buffSize < sendBuffSize) ? buffSize : sendBuffSize,
                        (int8_t *)buff, tid, /*waitForDone*/ false);

      // Get the second input and send to internal buffer 2 (NSP 2) for
      // processing
      buffNum = inputBufferNum(1);
      waitForBuffer(buffNum, /*clear*/ false);
      buffSize = getBufferSize(buffNum);
      buff = (char *)getBufferAddr(buffNum);

      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "Input Buffer 1 is ready and says: %s", buff);
      sendBuffNum = internalBufferNum(2);
      sendBuffSize = getBufferSize(sendBuffNum);
      broadcastToBuffer(sendBuffNum, 0,
                        (buffSize < sendBuffSize) ? buffSize : sendBuffSize,
                        (int8_t *)buff, tid, /*waitForDone*/ false);
    } else if (tid == 1) {
      // NSP 0 Thread 1 - wait for result, and send it to host

      // Wait for data from NSP 1 (coming in on internal buffer 0) &
      // NSP 2 (coming in on internal buffer 3)
      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "Waiting for Internal Buffer 0");
      waitForBuffer(internalBufferNum(0), /*clear*/ true);
      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "Internal Buffer 0 is ready and says: %s",
             (char *)getBufferAddr(internalBufferNum(0)));
      waitForBuffer(internalBufferNum(3), /*clear*/ true);
      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "Internal Buffer 3 is ready and says: %s",
             (char *)getBufferAddr(internalBufferNum(3)));

      // We overlapped the internal buffers onto the output buffer, so no need
      // to copy data
      // Send the output
      sendAllOutputs(/*waitForArrival*/ false, /*clear*/ true);
    } else {
      // NSP 0 - other threads do nothing
      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "NSP %d thread %d has nothing to do!", qctx->virtualNSPId, tid);
    }
  } else if (qctx->virtualNSPId <= 2) {
    if (tid == 0) {
      // NSP 1 & NSP 2 Thread 0 process the buffers NSP 0 sends them, and send
      // the result back
      parseBuffer(qctx, tid);
    } else {
      // NSP 1 & NSP 2 - other threads do nothing
      NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
             "NSP %d thread %d has nothing to do!", qctx->virtualNSPId, tid);
    }
  } else {
    // Other NSPs do nothing
    NN_LOG(qctx->logFuncPtr, NNC_LOG_MASK_INFO,
           "NSP %d thread %d has nothing to do!", qctx->virtualNSPId, tid);
  }
}
