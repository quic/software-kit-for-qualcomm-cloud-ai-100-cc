// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <gtest/gtest.h>

#include "toolchain/Compiler.h"

using namespace qaic;

TEST(Toolchain, Compiler_HexagonProcConfigDefaults) {
  Compiler::HexagonProcConfig defaultProcConfig;
  EXPECT_FALSE(defaultProcConfig.HVXEnable);
  EXPECT_FALSE(defaultProcConfig.HMXEnable);
  EXPECT_FALSE(defaultProcConfig.HVXIeeeFpEnable);
}

TEST(Toolchain, Compiler_HexagonProcConfigGetCommandLineArgs) {

  Compiler::HexagonProcConfig defaultProcConfig;

  auto args = defaultProcConfig.getCommandLine();
  EXPECT_EQ(2, args.size());

  Compiler::HexagonProcConfig allEnabled;
  allEnabled.HVXEnable = true;
  allEnabled.HMXEnable = true;
  allEnabled.HVXIeeeFpEnable = true;
  args = allEnabled.getCommandLine();
  EXPECT_EQ(5, args.size());
  for (auto &a : args) {
    llvm::errs() << a << "\n";
  }
}
