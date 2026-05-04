// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"

#include "Driver.h"
#include "DriverOptions.h"

using namespace llvm::opt;
using namespace qaic;
using namespace qaic::options;

#if LLVM_VERSION_MAJOR >= 16
#define PREFIX(NAME, VALUE) static constexpr llvm::StringLiteral NAME[] = VALUE;
#else
#define PREFIX(NAME, VALUE) static const char *const NAME[] = VALUE;
#endif
#include "DriverOptions.inc"
#undef PREFIX

static constexpr OptTable::Info InfoTable[] = {
#if LLVM_VERSION_MAJOR >= 18
#define OPTION(...) LLVM_CONSTRUCT_OPT_INFO(__VA_ARGS__),
#else
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  {PREFIX, NAME,  HELPTEXT,    METAVAR,     OPT_##ID,  Option::KIND##Class,    \
   PARAM,  FLAGS, OPT_##GROUP, OPT_##ALIAS, ALIASARGS, VALUES},
#endif
#include "DriverOptions.inc"
#undef OPTION
};

#if LLVM_VERSION_MAJOR >= 16
static constexpr llvm::StringLiteral PrefixesUnion[] = {
    llvm::StringLiteral("-"),
    llvm::StringLiteral("--"),
};
#endif

namespace {

class DriverOptTable final : public OptTable {
public:
  DriverOptTable() : OptTable(InfoTable) {}

#if LLVM_VERSION_MAJOR >= 16
private:
  llvm::ArrayRef<llvm::StringLiteral> getPrefixesUnion() const override {
    return PrefixesUnion;
  }
#endif
};
} // namespace

const OptTable &Driver::getDriverOptTable() {
  static DriverOptTable Table;
  return Table;
}
