// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <gtest/gtest.h>

#include "support/Path.h"

using namespace qaic;

TEST(Support, Path_SplitExtBasic) {
  std::string base, ext;

  std::tie(base, ext) = splitExtension("network.o");
  EXPECT_EQ("network", base);
  EXPECT_EQ(".o", ext);

  std::tie(base, ext) = splitExtension("/base/file.txt");
  EXPECT_EQ("/base/file", base);
  EXPECT_EQ(".txt", ext);

  std::tie(base, ext) = splitExtension("foo/base/hello.cc");
  EXPECT_EQ("foo/base/hello", base);
  EXPECT_EQ(".cc", ext);
}

TEST(Support, Path_SplitExtNoExt) {
  std::string base, ext;

  std::tie(base, ext) = splitExtension("network");
  EXPECT_EQ("network", base);
  EXPECT_EQ("", ext);

  std::tie(base, ext) = splitExtension("/base/file");
  EXPECT_EQ("/base/file", base);
  EXPECT_EQ("", ext);

  std::tie(base, ext) = splitExtension("foo/base/hello");
  EXPECT_EQ("foo/base/hello", base);
  EXPECT_EQ("", ext);
}

TEST(Support, Path_SplitExtNoBase) {
  std::string base, ext;

  std::tie(base, ext) = splitExtension(".o");
  EXPECT_EQ("", base);
  EXPECT_EQ(".o", ext);

  std::tie(base, ext) = splitExtension(".txt");
  EXPECT_EQ("", base);
  EXPECT_EQ(".txt", ext);

  std::tie(base, ext) = splitExtension(".cc");
  EXPECT_EQ("", base);
  EXPECT_EQ(".cc", ext);
}

TEST(Support, Path_SplitExtEmpty) {
  std::string base, ext;

  std::tie(base, ext) = splitExtension("");
  EXPECT_EQ("", base);
  EXPECT_EQ("", ext);
}
