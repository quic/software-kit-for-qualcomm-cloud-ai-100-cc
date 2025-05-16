// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "StringList.h"

using namespace qaic;

std::vector<llvm::StringRef> StringList::getRefList() const {
  std::vector<llvm::StringRef> list;
  for (auto &S : strings_) {
    list.emplace_back(S);
  }
  return list;
}
