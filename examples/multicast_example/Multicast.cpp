// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "MulticastLib.h"

extern "C" {
void activate(void *ctx, uint8_t virtualThreadId, uint32_t stid);
}

void activate(void *ctx, uint8_t virtualThreadId, uint32_t stid) {
  // Call the library function
  run((AICExecContext *)ctx, virtualThreadId, stid);
}
