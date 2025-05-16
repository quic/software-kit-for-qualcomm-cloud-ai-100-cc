// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_SUPPORT_PATH_H_
#define _QAIC_SUPPORT_PATH_H_

#include <string>
#include <utility>

#include "llvm/ADT/StringRef.h"

namespace qaic {

std::pair<std::string, std::string> splitExtension(llvm::StringRef path);

std::string joinPath(llvm::StringRef path, llvm::StringRef a = "",
                     llvm::StringRef b = "", llvm::StringRef c = "",
                     llvm::StringRef d = "");
} // namespace qaic

#endif
