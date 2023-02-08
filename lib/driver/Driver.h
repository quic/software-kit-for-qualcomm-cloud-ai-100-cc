// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLS_DRIVER_H_
#define _QAIC_TOOLS_DRIVER_H_

#include "llvm/Option/ArgList.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"

#include "DriverOptions.h"
#include "program/Program.h"
#include "program/ProgramConfig.h"

namespace qaic {

/**
 * @brief A driver for running multi-step compilation and linking of QAIC
 * applications.
 */
class Driver {
public:
  /**
   * @brief Specifies the mode of operation for the driver.
   */
  enum DriverMode {
    Unknown,
    PreProcess,       //< Runs the preprocessor only
    Compile,          //< Builds assembly code
    Assemble,         //< Builds an object file
    Link,             //< Builds an ELF or archive
    LinkWithMetadata, //< Builds and ELF and embeds metadata
    QPC               //< Builds a runnable QPC application
  };

  /**
   * @brief Utility for computing the output file names for artifacts
   */
  class OutputNames {
  public:
    explicit OutputNames(llvm::StringRef provided);

    /**
     * @brief Get the standard output name for pre-processing.
     *
     * @return std::string <file>.i
     */
    std::string getPreProcessStepOutputName() const;

    /**
     * @brief Gets the standard output name for source compilation.
     *
     * @return std::string <file>.s
     */
    std::string getCompileStepOutputName() const;

    /**
     * @brief Gets the standard output name for assembler step.
     *
     * @return std::string <file>.o
     */
    std::string getAssembleStepOutputName() const;

    /**
     * @brief Gets the standard output name for linking step.
     *
     * @return std::string <file>.elf
     */
    std::string getLinkStepOutputName() const;

    /**
     * @brief Gets the standard output name for QPC generation step.
     *
     * @return std::string <file>.qpc
     */
    std::string getQPCStepOutputName() const;

    /**
     * @brief Gets the standard output name for metadata binaries.
     *
     * @return std::string <file>.metadata.bin
     */
    std::string getMetadataOutputName() const;

    /**
     * @brief Gets the standard output name for network descriptor binaries.
     *
     * @return std::string <file>.netdesc.bin
     */
    std::string getNetworkDescriptorOutputName() const;

    /**
     * @brief Gets the original file path given.
     */
    llvm::StringRef getProvidedName() const { return provided_; }

  private:
    std::string provided_;
  };

  /**
   * @brief Construct a new Driver object.
   *
   * Argv[0] must be the path to the tool instantiating the Driver (i.e.
   * qaic-cc)
   *
   * @param Argv Command line arguments passed to main
   */
  explicit Driver(llvm::ArrayRef<const char *> Argv)
      : RawDriverArgv_(Argv), Mode_(Unknown) {}

  /**
   * @brief Parses the command line arguments into an InputArgList
   *
   * @param MissingArgIndex On error, the index of the option which could not be
   * parsed
   * @param MissingArgCount On error, the number of missing options
   * @return reference to the argument list
   */
  const llvm::opt::InputArgList &parseArgs(unsigned &MissingArgIndex,
                                           unsigned &MissingArgCount);

  /**
   * @brief Gets the parse argument list
   */
  const llvm::opt::InputArgList &getParsedArgs() const {
    return ParsedDriverArgs_;
  }

  /**
   * @brief Prints the comnmand line help message to stdout.
   */
  void printHelp();

  /**
   * @brief Deduces the mode of operation from the parsed command line
   * arguments.
   */
  bool deduceDriverMode();

  /**
   * @brief Returns true if the driver is invoked in a mode that only produces
   * an ELF
   */
  bool isLinkOnlyMode() const {
    return Mode_ == Link || Mode_ == LinkWithMetadata;
  }

  /**
   * @brief Gets the driver mode.
   */
  DriverMode getDriverMode() const { return Mode_; }

  /**
   * @brief Gets the root path of the QAIC compute toolchain.
   */
  llvm::StringRef getInstallRootPath() const { return installRoot_; }

  /**
   * @brief Gets the path to where QAIC compute libraries (i.e. qaicrt) are
   * located.
   */
  llvm::StringRef getInstallLibPath() const { return installLibPath_; }

  /**
   * @brief Gets the path to where QAIC compute library headers are located.
   */
  llvm::StringRef getInstallIncludePath() const { return installIncludePath_; }

  /**
   * @brief Runs the driver and executes the actions appropriate for the mode of
   * execution.
   */
  bool run();

private:
  const llvm::opt::OptTable &getDriverOptTable();

  DriverMode Mode_;

  llvm::opt::InputArgList ParsedDriverArgs_;
  llvm::ArrayRef<const char *> RawDriverArgv_;
  std::string installRoot_;
  std::string installLibPath_;
  std::string installIncludePath_;
};
} // namespace qaic

#endif
