// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Ar.h"
#include "support/StringList.h"

using namespace llvm;
using namespace qaic;

Ar::Ar() : ToolBase("ar"), op_(UNSPECIFIED_OP) {}

int Ar::execute() {
  auto pathOrError = getTools().findAr();
  if (std::error_code ec = pathOrError.getError()) {
    REPORT_TOOL_ERROR("can't find ar tool.\n");
    return ec.value();
  }

  StringList args;
  args.push_back(pathOrError.get());

  std::string spec;
  switch (op_) {
  case DELETE:
    spec += "d";
    break;
  case PRINT:
    spec += "p";
    break;
  case REPLACE:
    spec += "r";
    break;
  case EXTRACT:
    spec += "x";
    break;
  default:
    REPORT_TOOL_ERROR("no operation specified.\n");
    return 1;
  }

  if (archiveFile_.empty()) {
    REPORT_TOOL_ERROR("missing archive file.\n");
    return 1;
  }

  for (auto mod : mods_) {
    switch (mod) {
    case CREATE:
      spec += "c";
      break;
    case INDEX:
      spec += "s";
      break;
    case NOINDEX:
      spec += "S";
      break;
    case UPDATE_RECENT:
      spec += "u";
      break;
    case COUNT:
      spec += "N";
      break;
    default:
      llvm::outs() << "Skipping unknown mod for Ar";
      break; // don't add anything
    }
  }

  args.push_back(spec);
  if (count_ != 0)
    args.push_back(std::to_string(count_));
  args.push_back(archiveFile_);
  for (auto &f : inputFiles_) {
    args.push_back(f);
  }

  return runProcessWithArgs(pathOrError.get(), args.getRefList());
}
