// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "driver/Driver.h"
#include "support/Debug.h"

using namespace qaic;
using namespace llvm;

int main(int argc, const char *argv[]) {
  ArrayRef<const char *> Argv{argv, static_cast<size_t>(argc)};
  Driver theDriver{Argv};

  initDebugging();

  unsigned MissingArgIndex, MissingArgCount;
  auto &Args = theDriver.parseArgs(MissingArgIndex, MissingArgCount);

  if (Args.hasArg(options::OPT_help)) {
    theDriver.printHelp();
    return 0;
  }

  initDebugging(Args.hasArg(options::OPT_Debug));

  if (!theDriver.deduceDriverMode())
    return 255;

  return theDriver.run() ? 0 : 99;
}
