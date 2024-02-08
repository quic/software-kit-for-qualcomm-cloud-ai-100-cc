// Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "support/Debug.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"

#include "Tools.h"

using namespace llvm;

#define DEBUG_TYPE "toolchain"

bool qaic::Toolset::isEmptyToolset() const {
  return (!CC.hasValue() && !CXX.hasValue() && !LD.hasValue() &&
          !AR.hasValue() && !Objcopy.hasValue());
}

void qaic::Tools::loadStandardEnvPaths() {
  // Add hexagon tools dir if defined
  auto hexToolsDir = sys::Process::GetEnv("HEXAGON_TOOLS_DIR");
  if (hexToolsDir.hasValue()) {
    setHexagonToolsPath(hexToolsDir.getValue() + "/bin");
  }

  // Add PATHs to tool search
  auto pathEnv = sys::Process::GetEnv("PATH");
  if (pathEnv.hasValue()) {
    SmallVector<StringRef, 16> paths;
    SplitString(pathEnv.getValue(), paths,
                std::string("") + sys::EnvPathSeparator);
    for (auto &s : paths) {
      addSearchPath(s);
    }
  }
}

void qaic::Tools::setHexagonToolsPath(llvm::StringRef path) {
  QAIC_DEBUG_STREAM("Setting hexagon tools path to " << path << "\n");
  hexagonToolsPath_ = path;
}

void qaic::Tools::addSearchPath(llvm::StringRef path) {
  QAIC_DEBUG_STREAM("Adding tool search path " << path << "\n");
  searchPaths_.push_back(path.str());
}

std::vector<StringRef> qaic::Tools::getSearchPaths() const {

  std::vector<StringRef> paths;

  // Ensure the current working directory is added
  // We need to cache it because this function returns string refs.
  if (currentDirCached_.empty()) {
    SmallString<128> current_dir;
    std::error_code ec = sys::fs::current_path(current_dir);
    if (!ec) {
      currentDirCached_ = current_dir.str();
    }
  }

  if (!currentDirCached_.empty())
    paths.emplace_back(currentDirCached_);

  // Hexagon search path should always take full precedence
  if (!hexagonToolsPath_.empty())
    paths.emplace_back(hexagonToolsPath_);

  // Add any provided search paths
  for (auto &p : searchPaths_) {
    paths.emplace_back(p);
  }

  return paths;
}

ErrorOr<std::string>
qaic::Tools::findProgramByName(StringRef program,
                               ArrayRef<StringRef> searchPaths) const {
  QAIC_DEBUG_STREAM("Searching for tool " << program << "\n");
  ErrorOr<std::string> res = sys::findProgramByName(program, searchPaths);
  QAIC_DEBUG(if (res) {
    llvm::dbgs() << "Found tool " << program << " at " << res.get() << "\n";
  });
  return res;
}

ErrorOr<std::string> qaic::Tools::findCCompiler() const {
  std::vector<StringRef> paths = getSearchPaths();
  return findProgramByName(hexagonToolset_.CC.getValueOr("clang"), paths);
}

ErrorOr<std::string> qaic::Tools::findCXXCompiler() const {
  std::vector<StringRef> paths = getSearchPaths();
  return findProgramByName(hexagonToolset_.CXX.getValueOr("clang++"), paths);
}

ErrorOr<std::string> qaic::Tools::findLinker() const {
  std::vector<StringRef> paths = getSearchPaths();
  return findProgramByName(hexagonToolset_.LD.getValueOr("llvm-link"), paths);
}

ErrorOr<std::string> qaic::Tools::findAr() const {
  std::vector<StringRef> paths = getSearchPaths();
  return findProgramByName(hexagonToolset_.AR.getValueOr("llvm-ar"), paths);
}

ErrorOr<std::string> qaic::Tools::findObjcopy() const {
  std::vector<StringRef> paths = getSearchPaths();
  return findProgramByName(
        hexagonToolset_.Objcopy.getValueOr("llvm-objcopy"), paths);
}
