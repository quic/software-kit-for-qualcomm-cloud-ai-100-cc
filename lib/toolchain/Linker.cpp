// Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <sstream>

#include "Ar.h"
#include "Linker.h"
#include "Tools.h"
#include "support/Debug.h"
#include "support/StringList.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"

using namespace llvm;

#define DEBUG_TYPE "toolchain"

constexpr uintptr_t qaic::Linker::DEFAULT_TEXT_SECTION_ADDR;
constexpr uintptr_t qaic::Linker::DEFAULT_STACK_SIZE;
const char qaic::Linker::TEXT_SECTION[] = ".text";
const char qaic::Linker::START_SECTION[] = ".start";
const char qaic::Linker::HEXAGON_STACK_SIZE_SYM[] = "STACK_SIZE";

qaic::Linker::Linker()
    : ToolBase("Linker"), group_(false), wholeArchive_(false),
      linkStandardLibs_(false), verbose_(false) {}

void qaic::Linker::startGroup() {
  assert(!group_ && "mismatched call to start/endGroup");
  if (!group_) {
    linkerLibs_.emplace_back("-Wl,--start-group");
    group_ = true;
  }
}

void qaic::Linker::endGroup() {
  assert(group_ && "mismatched call to start/endGroup");
  if (group_) {
    linkerLibs_.emplace_back("-Wl,--end-group");
    group_ = false;
  }
}

void qaic::Linker::startWholeArchive() {
  assert(!wholeArchive_ && "mismatched call to start/endWholeArchive");
  if (!wholeArchive_) {
    linkerLibs_.emplace_back("-Wl,--whole-archive");
    wholeArchive_ = true;
  }
}

void qaic::Linker::endWholeArchive() {
  assert(wholeArchive_ && "mismatched call to start/endWholeArchive");
  if (wholeArchive_) {
    linkerLibs_.emplace_back("-Wl,--no-whole-archive");
    wholeArchive_ = false;
  }
}

void qaic::Linker::addLinkerSearchPath(llvm::StringRef path) {
  if (path.startswith("-Wl,-L")) {
    path.consume_front("-Wl,-L");
  }
  std::string arg = "-Wl,-L";
  arg += path;
  linkerLibs_.push_back(std::move(arg));
  QAIC_DEBUG_STREAM("adding linker search path: " << path << "\n");
}

void qaic::Linker::addLib(llvm::StringRef lib) {
  std::string arg = "-l";
  arg += lib;
  linkerLibs_.push_back(std::move(arg));
  QAIC_DEBUG_STREAM("adding linker lib: " << lib << "\n");
}

uintptr_t qaic::Linker::getSectionAddr(llvm::StringRef sectionName) const {
  if (sectionAddrs_.count(sectionName.str())) {
    return sectionAddrs_.at(sectionName.str());
  } else {
    return 0;
  }
}

int qaic::Linker::execute() {

  auto pathOrError = getTools().findCXXCompiler();
  if (std::error_code ec = pathOrError.getError()) {
    REPORT_TOOL_ERROR("can't find linker.\n");
    return ec.value();
  }

  StringList args;
  args.push_back(pathOrError.get());
  for (auto &lib : linkerLibs_) {
    args.push_back(lib);
  }

  if (outputFile_.empty()) {
    REPORT_TOOL_ERROR("no output file given.\n");
    return 1;
  }

  args.push_back("-o");
  args.push_back(outputFile_);

  std::vector<std::string> tmpObjs;
  // Default to freestanding
  if (!linkStandardLibs_) {
    args.push_back("-nostdlib");
    args.push_back("-nostdlib++");
  }

  // Virtual addresses below a specific point are not valid
  args.push_back(
	llvm::formatv("-Wl,--image-base={0:x}", DEFAULT_TEXT_SECTION_ADDR)
            .str());

  // Hexagon supports 4K pages, and we would like to keep overall size down
  args.push_back(
        llvm::formatv("-Wl,-z,max-page-size=4096")
            .str());

  for (auto const &kvp : defSyms_) {
    args.push_back(
        llvm::formatv("-Wl,--defsym,{0}={1}", kvp.first, kvp.second).str());
  }

  if (verbose_) {
    args.push_back("-Wl,--verbose");
  }

  for (auto const &s : pretendUndefSyms_) {
    args.push_back("-u");
    args.push_back(s);
  }

  for (auto const &s : wrapSyms_) {
    args.push_back(llvm::formatv("-Wl,--wrap,{0}", s).str());
  }
  // FW expects the first segment to be RX which then allows the FW to
  // efficiently share the segment with all NSPs operating the workload.
  args.push_back("-Wl,--no-rosegment");
  args.push_back(llvm::formatv("-Wl,-z,separate-code").str());

  auto rc = runProcessWithArgs(pathOrError.get(), args.getRefList());
  if (!linkStandardLibs_) {
    for (auto const &fn : tmpObjs) {
      auto ec = sys::fs::remove(fn);
      if (ec) {
        llvm::errs() << "Failed to remove temp build file " << fn << "\n";
      }
    }
  }
  return rc;
}
