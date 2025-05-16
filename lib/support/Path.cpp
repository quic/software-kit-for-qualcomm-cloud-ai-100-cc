// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Path.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

using namespace llvm;

std::pair<std::string, std::string> qaic::splitExtension(StringRef path) {
  auto ext = sys::path::extension(path);
  auto stem = sys::path::stem(path);
  auto parent = sys::path::parent_path(path);

  SmallString<256> base;
  sys::path::append(base, parent, stem);
  return {base.c_str(), ext.str()};
}

std::string qaic::joinPath(StringRef path, StringRef a, StringRef b,
                           StringRef c, StringRef d) {
  SmallString<256> pathVec{path};
  sys::path::append(pathVec, a, b, c, d);
  return pathVec.str().str();
}
