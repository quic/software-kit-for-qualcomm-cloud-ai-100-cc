// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_APPCONFIG_H_
#define _QAIC_APPCONFIG_H_

#include <memory>

#include "ProgramConfig.pb.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"

namespace qaic {

/**
 * @brief Program configuration describing I/O and resources.
 *
 * The program configuration is serialized as JSON and loaded into
 * a protobuf message.
 */
class ProgramConfig {
public:
  ProgramConfig();
  ProgramConfig(ProgramConfig &&other) noexcept;

  bool loadFromFile(llvm::StringRef path);
  bool loadFromString(llvm::StringRef str);

  aicnwdesc::ProgramConfig &get() { return *message_; }
  const aicnwdesc::ProgramConfig &get() const { return *message_; }

private:
  std::unique_ptr<aicnwdesc::ProgramConfig> message_;
};
} // namespace qaic

#endif
