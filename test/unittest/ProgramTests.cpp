// Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "AICMetadataReader.h"
#include <fstream>
#include <gtest/gtest.h>

#include "program/Program.h"
#include "program/ProgramConfig.h"

#include "llvm/Support/Debug.h"

using namespace qaic;

TEST(Program, ProgramConfig_ConstructEmpty) {
  ProgramConfig cfg;
  EXPECT_EQ("", cfg.get().name());
  EXPECT_EQ(0, cfg.get().hwversionmajor());
  EXPECT_EQ(0, cfg.get().hwversionminor());
  EXPECT_EQ(0, cfg.get().numnsps());
  EXPECT_EQ(0, cfg.get().inputs_size());
  EXPECT_EQ(0, cfg.get().outputs_size());
}

TEST(Program, ProgramConfig_MoveConstruct) {
  ProgramConfig cfg;
  cfg.get().set_name("simple_app");
  cfg.get().set_numnsps(14);

  ProgramConfig other{std::move(cfg)};
  ASSERT_EQ("simple_app", other.get().name());
  ASSERT_EQ(14, other.get().numnsps());
}

TEST(Program, ProgramConfig_LoadFromJSONString) {
  std::string SimpleProgramConfig = "{\n"
                                    "name: \"simple_app\",\n"
                                    "hwVersionMajor: 2,\n"
                                    "hwVersionMinor: 0,\n"
                                    "numNSPs: 14\n"
                                    "}\n";

  ProgramConfig cfg;
  llvm::dbgs() << SimpleProgramConfig << "\n";
  ASSERT_TRUE(cfg.loadFromString(SimpleProgramConfig));
}

TEST(Program, ProgramConfig_LoadFromJSONFile) {
  std::string SimpleProgramConfig = "{\n"
                                    "name: \"simple_app\",\n"
                                    "hwVersionMajor: 2,\n"
                                    "hwVersionMinor: 0,\n"
                                    "numNSPs: 14\n"
                                    "}\n";

  std::ofstream ofs{"simple_app.json"};
  ofs << SimpleProgramConfig;
  ofs.close();

  ProgramConfig cfg;
  ASSERT_TRUE(cfg.loadFromFile("./simple_app.json"));
}

TEST(Program, ComputeProgram_Construct) {
  ProgramConfig config;
  ASSERT_TRUE(config.loadFromFile("test_program.json"));
  ComputeProgram program{std::move(config)};
}

TEST(Program, ComputeProgram_GenerateMetadata) {
  ProgramConfig config;
  ASSERT_TRUE(config.loadFromFile("test_program.json"));
  ComputeProgram program{std::move(config)};

  program.setEntrypointAddr(0xd00d7110);
  auto meta = program.generateMetadata();
  ASSERT_NE(nullptr, meta.get());

  // Dump it out for now we need to figure out a better way to test
  // Perhaps have a hand crafted metadata binary or something
  auto metabuf = meta->getMetadata();
  char errorBuf[256] = {0};
  const AICMetadata *metaPtr =
      MDR_readMetadata(&metabuf[0], metabuf.size(), errorBuf, sizeof(errorBuf));
  AICMetadata_dump(metaPtr);
}

TEST(Program, ComputeProgram_GenerateNetworkDcriptor) {
  ProgramConfig config;
  ASSERT_TRUE(config.loadFromFile("test_program.json"));
  ComputeProgram program{std::move(config)};

  program.setEntrypointAddr(0xd00d7110);
  auto netdesc = program.generateNetworkDescriptor();
  ASSERT_NE(nullptr, netdesc.get());
}

TEST(Program, ComputeProgram_ExampleConfig) {
  ProgramConfig config;
  ASSERT_TRUE(config.loadFromFile("example_config.json"));
  ComputeProgram program{std::move(config)};

  program.setEntrypointAddr(0x02000000);
  auto meta = program.generateMetadata();
  ASSERT_NE(nullptr, meta.get());

  // Dump it out for now we need to figure out a better way to test
  // Perhaps have a hand crafted metadata binary or something
  auto metabuf = meta->getMetadata();
  char errorBuf[256] = {0};
  const AICMetadata *metaPtr =
      MDR_readMetadata(&metabuf[0], metabuf.size(), errorBuf, sizeof(errorBuf));
  AICMetadata_dump(metaPtr);

  EXPECT_EQ(512 * 1024 * 1024, metaPtr->staticSharedDDRSize);
  EXPECT_EQ(8 * 1024 * 1024, metaPtr->VTCMSize);
  EXPECT_LT(1021 * 1024, metaPtr->L2TCMSize);
  EXPECT_EQ(14, metaPtr->numNSPs);
  EXPECT_EQ(2, metaPtr->numSemaphores);
  EXPECT_EQ(1120, metaPtr->exitDoorbellOffset);
  EXPECT_LT(128, metaPtr->L2TCMInitSize);
  EXPECT_EQ(4, metaPtr->numDMARequests);
  EXPECT_EQ(5, metaPtr->numThreadDescriptors);
}
