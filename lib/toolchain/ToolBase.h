// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLCHAIN_TOOLBASE_H_
#define _QAIC_TOOLCHAIN_TOOLBASE_H_

#include "llvm/Support/raw_ostream.h"

#include "Tools.h"

#define REPORT_TOOL_ERROR(X)                                                   \
  do {                                                                         \
    llvm::errs() << "[" << getToolName() << "] " << X;                         \
  } while (false)

namespace qaic {
class ToolBase {
public:
  ToolBase(llvm::StringRef toolName);

  Tools &getTools() { return tools_; }

  llvm::StringRef getToolName() const { return toolName_; }

  virtual int execute() = 0;

protected:
  virtual int runProcessWithArgs(llvm::StringRef arg0,
                                 llvm::ArrayRef<llvm::StringRef> args);

private:
  std::string toolName_;
  Tools tools_;
};
} // namespace qaic

#endif
