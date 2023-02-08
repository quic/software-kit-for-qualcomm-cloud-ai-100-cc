// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLS_DRIVER_ACTION_H_
#define _QAIC_TOOLS_DRIVER_ACTION_H_

#include "llvm/Option/ArgList.h"

#include "toolchain/Compiler.h"
#include "toolchain/Linker.h"
#include "toolchain/ObjCopy.h"

namespace qaic {

class Driver;
class DriverContext;

/**
 * @brief Base class for all actions executed by the Driver
 */
class DriverAction {
public:
  /**
   * @brief Construct a new Driver Action object
   *
   * @param D The Driver
   * @param name Name of the action that will be used for logging
   */
  DriverAction(Driver &D, llvm::StringRef name);

  virtual ~DriverAction() = default;

  /**
   * @brief Runs the action.
   */
  bool run(DriverContext &context);

  /**
   * @brief Get the Driver object
   */
  Driver &getDriver() { return D_; }

  /**
   * @brief Get the Driver object
   */
  const Driver &getDriver() const { return D_; }

  /**
   * @brief Get the parsed input arguments set in the Driver
   */
  const llvm::opt::InputArgList &getDriverArgs() const;

  /**
   * @brief Get the action name.
   */
  llvm::StringRef getActionName() const { return actionName_; }

protected:
  /**
   * @brief Performs pre-run check step for the action.
   */
  virtual bool preRunCheckImpl(DriverContext &context) const { return true; }

  /**
   * @brief Implementation of the run function for concrete actions.
   */
  virtual bool runImpl(DriverContext &context) = 0;

private:
  Driver &D_;
  std::string actionName_;
};

/**
 * @brief Executes the compiler to pre-process, compile and assemble source
 * code.
 */
class CompileAction : public DriverAction {
public:
  explicit CompileAction(Driver &D);
  virtual ~CompileAction() = default;

protected:
  bool preRunCheckImpl(DriverContext &context) const override;
  bool runImpl(DriverContext &context) override;

private:
  Compiler compiler_;
};

/**
 * @brief Executes the linker to join object files and archives produce an ELF
 */
class LinkAction : public DriverAction {
public:
  explicit LinkAction(Driver &D);
  virtual ~LinkAction() = default;

protected:
  bool runImpl(DriverContext &context) override;

private:
  Linker linker_;
};

/**
 * @brief Extracts the address of the entry point function from an elf.
 */
class ExtractEntryPointAddressAction : public DriverAction {
public:
  explicit ExtractEntryPointAddressAction(Driver &D);
  virtual ~ExtractEntryPointAddressAction() = default;

protected:
  bool preRunCheckImpl(DriverContext &context) const override;
  bool runImpl(DriverContext &context) override;
};

/**
 * @brief Generates metadata and embeds it into an ELF.
 */
class EmbedMetadataAction : public DriverAction {
public:
  explicit EmbedMetadataAction(Driver &D);
  virtual ~EmbedMetadataAction() = default;

protected:
  bool preRunCheckImpl(DriverContext &context) const override;
  bool runImpl(DriverContext &context) override;

private:
  ObjCopy objcopy_;
};

/**
 * @brief Generates a QPC and embeds the various segments required.
 */
class BuildQPCAction : public DriverAction {
public:
  explicit BuildQPCAction(Driver &D);
  virtual ~BuildQPCAction() = default;

protected:
  bool preRunCheckImpl(DriverContext &context) const override;
  bool runImpl(DriverContext &context) override;
};
} // namespace qaic

#endif
