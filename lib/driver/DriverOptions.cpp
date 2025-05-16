// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"

#include "Driver.h"
#include "DriverOptions.h"

using namespace llvm::opt;
using namespace qaic;
using namespace qaic::options;

#define PREFIX(NAME, VALUE) static const char *const NAME[] = VALUE;
#include "DriverOptions.inc"
#undef PREFIX

static const OptTable::Info InfoTable[] = {
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  {PREFIX, NAME,  HELPTEXT,    METAVAR,     OPT_##ID,  Option::KIND##Class,    \
   PARAM,  FLAGS, OPT_##GROUP, OPT_##ALIAS, ALIASARGS, VALUES},
#include "DriverOptions.inc"
#undef OPTION
};

namespace {

class DriverOptTable : public OptTable {
public:
  DriverOptTable() : OptTable(InfoTable) {}
};
} // namespace

const llvm::opt::OptTable &Driver::getDriverOptTable() {
  static DriverOptTable Table;
  return Table;
}
