// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"

#include "ObjCopy.h"
#include "support/StringList.h"

using namespace llvm;
using namespace qaic;

ObjCopy::ObjCopy()
    : ToolBase("ObjCopy"), stripMode_(None) {}

llvm::StringRef ObjCopy::sectionFlagToString(SectionFlag flag) const {
#define make_case(f)                                                           \
  case f:                                                                      \
    return #f

  switch (flag) {
    make_case(alloc);
    make_case(load);
    make_case(noload);
    make_case(readonly);
    make_case(exclude);
    make_case(debug);
    make_case(code);
    make_case(data);
    make_case(rom);
    make_case(share);
    make_case(contents);
    make_case(merge);
    make_case(strings);
  default:
    llvm_unreachable("invalid type");
  }

#undef make_case
}

int ObjCopy::execute() {
  auto pathOrError = getTools().findObjcopy();
  if (std::error_code ec = pathOrError.getError()) {
    REPORT_TOOL_ERROR("can't find objcopy.\n");
    return ec.value();
  }

  StringList args;
  args.push_back(pathOrError.get());

  if ((stripMode_ & All) == All) {
    args.push_back("--strip-all");
  }

  if ((stripMode_ & Debug) == Debug) {
    args.push_back("--strip-debug");
  }

  if ((stripMode_ & Unneeded) == Unneeded) {
    args.push_back("--strip-unneeded");
  }

  for (auto &kvp : sectionsToAdd_) {
    args.push_back(
        llvm::formatv("--add-section={0}={1}", kvp.first, kvp.second).str());
  }

  // Add section flags
  for (auto &kvp : sectionFlagsToSet_) {
    if (kvp.second.size() > 0) {
      SmallVector<StringRef, 4> flags;
      for (auto f : kvp.second) {
        flags.push_back(sectionFlagToString(f));
      }
      args.push_back(llvm::formatv("--set-section-flags={0}={1}", kvp.first,
                                   join(flags, ","))
                         .str());
    }
  }

  if (inputFile_.empty()) {
    REPORT_TOOL_ERROR("no input file given.\n");
    return 1;
  }

  args.push_back(inputFile_);

  if (!outputFile_.empty()) {
    args.push_back(outputFile_);
  }

  return runProcessWithArgs(pathOrError.get(), args.getRefList());
}
