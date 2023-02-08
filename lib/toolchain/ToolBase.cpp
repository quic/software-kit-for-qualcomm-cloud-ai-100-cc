// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "support/Debug.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include "ToolBase.h"

#define DEBUG_TYPE "toolchain"

using namespace llvm;
using namespace qaic;

ToolBase::ToolBase(StringRef toolName)
    : toolName_(toolName.str()) {}

int ToolBase::runProcessWithArgs(StringRef arg0, ArrayRef<StringRef> args) {

  QAIC_DEBUG_STREAM("exec: " << join(args, " ") << "\n");

  std::string errMsg;
  int rc = sys::ExecuteAndWait(arg0, args, {}, {}, 0, 0, &errMsg);
  if (rc != 0) {
    REPORT_TOOL_ERROR("Tool execution failed (rc=" << rc << ")\n");
    if (!errMsg.empty())
      REPORT_TOOL_ERROR(errMsg << "\n");
  } else {
    QAIC_DEBUG_STREAM("exec: returned 0\n");
  }

  return rc;
}
