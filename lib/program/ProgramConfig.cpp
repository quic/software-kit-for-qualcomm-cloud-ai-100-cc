// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ProgramConfig.h"
#include <google/protobuf/util/json_util.h>

using namespace llvm;
using namespace qaic;

ProgramConfig::ProgramConfig() : message_(new aicnwdesc::ProgramConfig) {}

ProgramConfig::ProgramConfig(ProgramConfig &&other) noexcept
    : message_(std::move(other.message_)) {}

bool ProgramConfig::loadFromFile(StringRef path) {
  auto BufOrErr = MemoryBuffer::getFileOrSTDIN(path);
  if (!BufOrErr) {
    llvm::errs() << "Unable to open expert config file for reading: " << path
                 << "\n";
    return false;
  }

  return loadFromString(BufOrErr.get()->getBuffer());
}

bool ProgramConfig::loadFromString(StringRef str) {
  ::google::protobuf::util::JsonParseOptions options;
  ::google::protobuf::StringPiece strPiece{str.data()};
  auto status = ::google::protobuf::util::JsonStringToMessage(
      strPiece, message_.get(), options);
  if (!status.ok()) {
    llvm::errs() << "Json Parsing Failed: " << status.ToString() << "\n";
    return false;
  }

  return true;
}
