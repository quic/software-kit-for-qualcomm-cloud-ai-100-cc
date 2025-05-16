// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLS_DRIVER_OPTIONS_H_
#define _QAIC_TOOLS_DRIVER_OPTIONS_H_

#include "llvm/Option/OptTable.h"
#include <memory>

namespace qaic {
namespace options {

/**
 * @brief ID numbers for Driver options.
 */
enum ID {
  OPT_INVALID = 0,
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  OPT_##ID,
#include "DriverOptions.inc"
#undef OPTION
  LastOption,
  OPT_NO_GROUP = OPT_INVALID,
  OPT_NO_ALIAS = OPT_INVALID

};
} // namespace options
} // namespace qaic

#endif
