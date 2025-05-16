// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <gtest/gtest.h>

#include "support/StringList.h"

using namespace qaic;

TEST(Support, StringList_Basic) {

  StringList list;
  EXPECT_EQ(0, list.size());
  EXPECT_TRUE(list.empty());

  list.push_back("");
  list.push_back("foo");
  list.push_back("bar");

  EXPECT_EQ(3, list.size());
  EXPECT_FALSE(list.empty());

  EXPECT_EQ("", list[0]);
  EXPECT_EQ("foo", list[1]);
  EXPECT_EQ("bar", list[2]);

  list.clear();
  EXPECT_EQ(0, list.size());
  EXPECT_TRUE(list.empty());
}

TEST(Support, StringList_Iterators) {
  StringList list;
  list.push_back("a");
  list.push_back("b");
  list.push_back("c");

  std::vector<std::string> other;
  for (auto &s : list) {
    other.push_back(s);
  }

  EXPECT_EQ("a", other[0]);
  EXPECT_EQ("b", other[1]);
  EXPECT_EQ("c", other[2]);
}

TEST(Support, StringList_GetRefList) {
  StringList list;
  list.push_back("1");
  list.push_back("2");
  list.push_back("3");

  std::vector<llvm::StringRef> other = list.getRefList();
  EXPECT_EQ(3, other.size());
  EXPECT_FALSE(other.empty());

  EXPECT_EQ("1", other[0]);
  EXPECT_EQ("2", other[1]);
  EXPECT_EQ("3", other[2]);
}
