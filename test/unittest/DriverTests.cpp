// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "driver/Driver.h"
#include <gtest/gtest.h>

using namespace qaic;

TEST(Driver, OutputNames) {
  auto check = [](Driver::OutputNames &outputs) {
    EXPECT_EQ("foo/bar/output.i", outputs.getPreProcessStepOutputName());
    EXPECT_EQ("foo/bar/output.s", outputs.getCompileStepOutputName());
    EXPECT_EQ("foo/bar/output.o", outputs.getAssembleStepOutputName());
    EXPECT_EQ("foo/bar/output.elf", outputs.getLinkStepOutputName());
    EXPECT_EQ("foo/bar/output.qpc", outputs.getQPCStepOutputName());

    EXPECT_EQ("foo/bar/output.metadata.bin", outputs.getMetadataOutputName());
    EXPECT_EQ("foo/bar/output.netdesc.bin",
              outputs.getNetworkDescriptorOutputName());
  };

  Driver::OutputNames outputs{"foo/bar/output.qpc"};
  EXPECT_EQ("foo/bar/output.qpc", outputs.getProvidedName());
  check(outputs);

  Driver::OutputNames outputs2{"foo/bar/output.o"};
  EXPECT_EQ("foo/bar/output.o", outputs2.getProvidedName());
  check(outputs2);

  Driver::OutputNames outputs3{"foo/bar/output"};
  EXPECT_EQ("foo/bar/output", outputs3.getProvidedName());
  check(outputs3);
}
