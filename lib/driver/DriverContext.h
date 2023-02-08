// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLS_DRIVER_CONTEXT_H_
#define _QAIC_TOOLS_DRIVER_CONTEXT_H_

#include <memory>
#include <set>
#include <vector>

#include "llvm/ADT/StringRef.h"

#include "DriverAction.h"
#include "program/Program.h"

namespace qaic {

class Driver;

/**
 * @brief Context for running the driver.
 */
class DriverContext {
public:
  explicit DriverContext(Driver &D) : Driver_(D) {}

  /**
   * @brief Registers a file as an intermediate file that should be removed upon
   * successful
   * completion of the driver actions.
   */
  void registerIntermediateFileForCleanup(llvm::StringRef file) {
    intermediateFiles_.insert(file);
  }

  /**
   * @brief Removes all files registered by \refitem
   * registerIntermediateFileForCleanup
   */
  bool cleanupIntermediateFiles() const;

  /**
   * @brief Sets the Program object for the context.
   */
  void setProgram(std::unique_ptr<Program> program) {
    program_ = std::move(program);
  }

  /**
   * @brief Gets the Program objects set in the context.
   *
   * If no Program object is set then return nullptr.
   */
  Program *getProgram() { return program_.get(); }

  /**
   * @brief Gets the Program objects set in the context.
   *
   * If no Program object is set then return nullptr.
   */
  const Program *getProgram() const { return program_.get(); }

  /**
   * @brief Gets a reference to the Driver that owns this context.
   */
  Driver &getDriver() { return Driver_; }

  /**
   * @brief Gets a reference to the Driver that owns this context.
   */
  const Driver &getDriver() const { return Driver_; }

  /**
   * @brief Adds an action to execute under this context.
   */
  void addAction(std::unique_ptr<DriverAction> action) {
    actionsToRun_.push_back(std::move(action));
  }

  /**
   * @brief Run the registered actions.
   *
   * If an error is encountered then no additional processing or cleanup
   * occurs.
   */
  bool run();

private:
  Driver &Driver_;
  std::set<std::string> intermediateFiles_;
  std::unique_ptr<Program> program_;
  std::vector<std::unique_ptr<DriverAction>> actionsToRun_;
};
} // namespace qaic

#endif
