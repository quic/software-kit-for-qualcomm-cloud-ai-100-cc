// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLCHAIN_OBJCOPY_H_
#define _QAIC_TOOLCHAIN_OBJCOPY_H_

#include <map>
#include <set>
#include <string>

#include "ToolBase.h"

namespace qaic {

class ObjCopy : public ToolBase {
public:
  enum StripSymbolMode {
    None = 0,
    All = 1,
    Debug = (1U << 1),
    Unneeded = (1U << 2)
  };

  enum SectionFlag {
    alloc,
    load,
    noload,
    readonly,
    exclude,
    debug,
    code,
    data,
    rom,
    share,
    contents,
    merge,
    strings
  };

  explicit ObjCopy();

  void setInputFile(llvm::StringRef file) { inputFile_ = file; }
  void setOutputFile(llvm::StringRef file) { outputFile_ = file; }
  void setStripSymbolsMode(StripSymbolMode mode) { stripMode_ = mode; }
  void setSectionFlag(llvm::StringRef sectionName, SectionFlag flag) {
    sectionFlagsToSet_[sectionName.str()].insert(flag);
  }
  void addSection(llvm::StringRef sectionName, llvm::StringRef path) {
    sectionsToAdd_[sectionName.str()] = path;
  }

  int execute() override;

private:
  llvm::StringRef sectionFlagToString(SectionFlag flag) const;

  std::string inputFile_;
  std::string outputFile_;
  StripSymbolMode stripMode_;
  std::map<std::string, std::string> sectionsToAdd_;
  std::map<std::string, std::set<SectionFlag>> sectionFlagsToSet_;
};
} // namespace qaic

#endif
