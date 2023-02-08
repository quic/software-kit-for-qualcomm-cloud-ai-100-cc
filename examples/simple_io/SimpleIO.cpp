// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SimpleIOLib.h"

extern "C" {
void activate(void *ctx, uint8_t virtualThreadId, uint32_t stid);
}

void activate(void *ctx, uint8_t virtualThreadId, uint32_t stid) {
  // Call the library function
  run((AICExecContext *)ctx, virtualThreadId, stid);
}
