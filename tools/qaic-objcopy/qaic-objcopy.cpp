// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "support/Debug.h"
#include "toolchain/Compiler.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

using namespace qaic;
using namespace llvm;

cl::opt<bool> Debug("debug", cl::desc("Enable debugging."), cl::ReallyHidden);
cl::list<std::string> PassthroughOptions(
    cl::desc("Options to pass through to the underlying tool."), cl::Sink);

int main(int argc, const char *argv[]) {
  cl::ParseCommandLineOptions(argc, argv);
  qaic::initDebugging(Debug);

  Tools tools;

  // Load standard environment paths for tools
  tools.loadStandardEnvPaths();

  auto objCopyPathOrError = tools.findObjcopy();
  if (std::error_code ec = objCopyPathOrError.getError()) {
    llvm::errs() << "can't find objcopy.\n";
    return -1;
  }

  std::vector<StringRef> args;
  args.push_back(objCopyPathOrError.get());

  for (auto &a : PassthroughOptions) {
    args.push_back(a);
  }

  std::string errMsg;
  int rc = sys::ExecuteAndWait(objCopyPathOrError.get(), args, {}, {}, 0, 0,
                               &errMsg);
  llvm::errs() << errMsg << "\n";
  return rc;
}
