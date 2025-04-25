// Copyright (c) 2021-2022, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLCHAIN_TOOLS_H_
#define _QAIC_TOOLCHAIN_TOOLS_H_

#include <string>
#include <vector>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorOr.h"

namespace qaic {

struct Toolset {
  llvm::Optional<std::string> CC;
  llvm::Optional<std::string> CXX;
  llvm::Optional<std::string> LD;
  llvm::Optional<std::string> AR;
  llvm::Optional<std::string> Objcopy;

  /// Returns true if no tool is set
  bool isEmptyToolset() const;
};

class Tools {
public:
  llvm::ErrorOr<std::string> findCCompiler() const;
  llvm::ErrorOr<std::string> findCXXCompiler() const;
  llvm::ErrorOr<std::string> findLinker() const;
  llvm::ErrorOr<std::string> findAr() const;
  llvm::ErrorOr<std::string> findObjcopy() const;

  Toolset &getHostToolset() { return hostToolset_; }
  const Toolset &getHostToolset() const { return hostToolset_; }

  Toolset &getHexagonToolset() { return hexagonToolset_; }
  const Toolset &getHexagonToolset() const { return hexagonToolset_; }

  void setHexagonToolsPath(llvm::StringRef path);
  llvm::StringRef getHexagonToolsPath() const { return hexagonToolsPath_; }

  void addSearchPath(llvm::StringRef path);

  void loadStandardEnvPaths();

private:
  llvm::ErrorOr<std::string>
  findProgramByName(llvm::StringRef program,
                    llvm::ArrayRef<llvm::StringRef> searchPaths) const;

  mutable std::string currentDirCached_;
  std::string hexagonToolsPath_;
  std::vector<std::string> searchPaths_;
  std::vector<llvm::StringRef> getSearchPaths() const;
  Toolset hostToolset_;
  Toolset hexagonToolset_;
};
} // namespace qaic

#endif
