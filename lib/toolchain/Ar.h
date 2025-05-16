// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLCHAIN_AR_H_
#define _QAIC_TOOLCHAIN_AR_H_

#include <string>
#include <vector>

#include "ToolBase.h"

namespace qaic {

/**
 * @brief The archiver utility.
 */
class Ar : public ToolBase {
public:
  /**
   * @brief Archiver operations.
   */
  enum Operation {
    UNSPECIFIED_OP, //< Unspecified.
    DELETE,         //< Delete from the archive.
    PRINT,          //< Print contents of the archive.
    REPLACE,        //< Replace or insert contents of the archive.
    EXTRACT         //< Extract contents of the archive.
  };

  enum OperationModifier {
    UNSPECIFIED_MOD, //< Unspecified.
    CREATE,          //< Creates the archive if it does not exist.
    INDEX,           //< Generates and index/symbol table.
    NOINDEX,         //< Remove the index/symbol table,
    UPDATE_RECENT,   //< Only update entries with more recent timestamps.
    COUNT,           //< Sets the count value
  };

  explicit Ar();

  /**
   * @brief Set the path to the archive file.
   */
  void setArchiveFile(llvm::StringRef file) { archiveFile_ = file; }

  /**
   * @brief Adds an input file to the command.
   */
  void addInputFile(llvm::StringRef file) { inputFiles_.push_back(file.str()); }

  /**
   * @brief Clears input file list for the command.
   */
  void clearInputFiles() { inputFiles_.clear(); }

  /**
   * @brief Sets the operation to perform.
   */
  void setOption(Operation op) { op_ = op; }

  /**
   * @brief Adds an operation modifier.
   */
  void addOperationModifier(OperationModifier mod) {
    if (mod != COUNT) {
      mods_.push_back(mod);
    } else {
      llvm::errs() << "COUNT modifier should be specified with setCount";
      exit(-1);
    }
  }

  /**
   * @brief Clears modifier list for the command.
   */
  void clearOperationModifier(OperationModifier mod) {
    mods_.clear();
    count_ = 0;
  }

  /**
   * @brief Set the count for the command.
   */
  void setCount(int count) {
    if (count == 0) {
      llvm::errs() << "Count value must not be zero. Use "
                      "clearOperationModifier() to clear count modifier";
      exit(-1);
    }
    if (count_ == 0) {
      // Only add the modifier the first time count is set
      mods_.push_back(COUNT);
    }
    count_ = count;
  }

  /**
   * @brief Executes the tool and returns a process exit code.
   * @return int 0 for sucess, non zero for error
   */
  int execute() override;

private:
  Operation op_;
  std::vector<OperationModifier> mods_;
  std::string archiveFile_;
  std::vector<std::string> inputFiles_;
  int count_{0};
};
} // namespace qaic

#endif
