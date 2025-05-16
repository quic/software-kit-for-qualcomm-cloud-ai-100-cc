// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <gtest/gtest.h>

#include "toolchain/Tools.h"
#include "llvm/Support/Host.h"

using namespace qaic;

TEST(Toolchain, Tools_Constructor) {
  qaic::Tools tools;
  EXPECT_TRUE(tools.getHexagonToolsPath().empty());
  EXPECT_TRUE(tools.getHostToolset().isEmptyToolset());
  EXPECT_TRUE(tools.getHexagonToolset().isEmptyToolset());
}

TEST(Toolchain, Tools_Toolset) {
  qaic::Toolset defaultTs;
  EXPECT_TRUE(defaultTs.isEmptyToolset());

  qaic::Toolset customTs;
  customTs.CXX = "/usr/bin/g++";
  EXPECT_FALSE(customTs.isEmptyToolset());
}

TEST(Toolchain, Tools_FindToolsNoPaths) {
  qaic::Tools t;

  // If not paths are given then only the cwd is used
  // so we should never find any tools
  auto testForTarget = [&]() {
    auto cc = t.findCCompiler();
    EXPECT_FALSE(cc);

    auto cxx = t.findCXXCompiler();
    EXPECT_FALSE(cxx);

    auto ld = t.findLinker();
    EXPECT_FALSE(ld);

    auto ar = t.findAr();
    EXPECT_FALSE(ar);

    auto objc = t.findObjcopy();
    EXPECT_FALSE(objc);
  };

  testForTarget();
}

TEST(Toolchain, Tools_FindHexagonTools) {

  auto findMockTools = [&](qaic::Tools &t) {
    auto cc = t.findCCompiler();
    EXPECT_TRUE(cc);
    EXPECT_EQ("./mock_tools/hexagon/clang", cc.get());

    auto cxx = t.findCXXCompiler();
    EXPECT_TRUE(cxx);
    EXPECT_EQ("./mock_tools/hexagon/clang++", cxx.get());

    auto ld = t.findLinker();
    EXPECT_TRUE(ld);
    EXPECT_EQ("./mock_tools/hexagon/llvm-link", ld.get());

    auto ar = t.findAr();
    EXPECT_TRUE(ar);
    EXPECT_EQ("./mock_tools/hexagon/llvm-ar", ar.get());

    auto objc = t.findObjcopy();
    EXPECT_TRUE(objc);
    EXPECT_EQ("./mock_tools/hexagon/llvm-objcopy", objc.get());
  };

  // Hexagon tools can be found by explicitly setting the path
  qaic::Tools toolsHexPath;
  toolsHexPath.setHexagonToolsPath("./mock_tools/hexagon");
  findMockTools(toolsHexPath);

  // Tools can be found via regular path as well
  qaic::Tools toolsSysPath;
  toolsSysPath.addSearchPath("./mock_tools/hexagon");
  findMockTools(toolsSysPath);

  // Tools can also be explicitly set via toolset
  qaic::Tools toolsetPaths;
  toolsetPaths.getHexagonToolset().CC = "./mock_tools/hexagon/clang";
  toolsetPaths.getHexagonToolset().CXX = "./mock_tools/hexagon/clang++";
  toolsetPaths.getHexagonToolset().LD = "./mock_tools/hexagon/llvm-link";
  toolsetPaths.getHexagonToolset().AR = "./mock_tools/hexagon/llvm-ar";
  toolsetPaths.getHexagonToolset().Objcopy =
      "./mock_tools/hexagon/llvm-objcopy";
  findMockTools(toolsetPaths);
}
