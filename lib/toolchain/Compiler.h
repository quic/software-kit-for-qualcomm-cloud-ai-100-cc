// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLCHAIN_COMPILER_H_
#define _QAIC_TOOLCHAIN_COMPILER_H_

#include "llvm/Support/Error.h"

#include "ToolBase.h"

namespace qaic {

/**
 * @brief Wrapper for the compiler tool.
 */
class Compiler : public ToolBase {
public:
  /**
   * @brief Mode of compilation.
   */
  enum CompilerMode {
    Unknown,
    PreProcess,        //< Run the pre-processor only
    CompileNoAssemble, //< Run the pre-processors and compile source
    Compile,           //< Run the pre-processors, compile source and assemble
  };

  /**
   * @brief Compiler optimization levels.
   */
  enum OptLevel { O0, O1, O2, O3 };

  /**
   * @brief C++ standard
   */
  enum CxxStd { cxx_unknown = 0, cxx11 = 11, cxx14 = 14, cxx17 = 17 };

  /**
   * @brief Hexagon accelerator configuration
   */
  struct HexagonProcConfig {
    bool HVXEnable{false};       //< Enable the HVX
    bool HVXIeeeFpEnable{false}; //< Enable IEEE floating point with HVX
    bool HMXEnable{false};       //< Enable the HMX

    /**
     * @brief Gets the command line flags appropriate for the configuration.
     * @param t The target architecture.
     */
    std::vector<std::string> getCommandLine() const;
  };

  explicit Compiler();

  /**
   * @brief Sets the compilation mode.
   */
  void setMode(CompilerMode mode) { compilerMode_ = mode; }

  /**
   * @brief Gets the compilation mode.
   */
  CompilerMode getMode() const { return compilerMode_; }

  /**
   * @brief Set the path to the input source file to compile.
   */
  void setSourceFile(llvm::StringRef file) { sourceFile_ = file; }

  /**
   * @brief Get the path to the input source file to compile.
   */
  llvm::StringRef getSourceFile() const { return sourceFile_; }

  /**
   * @brief Set the path to the output file of the compiler.
   */
  void setOutputFile(llvm::StringRef file) { outputFile_ = file; }

  /**
   * @brief Get the path to the output file of the compiler.
   */
  llvm::StringRef getOutputFile() const { return outputFile_; }

  /**
   * @brief Add an include search path.
   */
  void addIncludePath(llvm::StringRef path);

  /**
   * @brief Adds a system include search path.
   */
  void addSystemIncludePath(llvm::StringRef path);

  /**
   * @brief Defines a macro.
   */
  void addDefine(llvm::StringRef def);

  /**
   * @brief Undefines a macro.
   */
  void addUndefine(llvm::StringRef def);

  /**
   * @brief Sets the optimization level.
   */
  void setOptLevel(OptLevel level) { optLevel_ = level; }

  /**
   * @brief Gets the optmization level.
   */
  OptLevel getOptLevel() const { return optLevel_; }

  /**
   * @brief Enables generation of debug information.
   */
  void setDebugFlag(bool flag) { debugFlag_ = flag; }

  /**
   * @brief Return if debug information will be generated.
   */
  bool getDebgFlag() const { return debugFlag_; }

  /**
   * @brief If enabled saves temporary outputs from the compilation.
   */
  void setSaveTempsFlag(bool flag) { saveTemps_ = flag; }

  /**
   * @brief Return if temporary outputs from compilation will be saved.
   */
  bool getSaveTempsFlag() const { return saveTemps_; }

  /**
   * @brief Adds a warning flag/option to the compilation.
   */
  void addWarningFlag(llvm::StringRef flag);

  /**
   * @brief Set the C++ standard to compile for.
   */
  void setCxxStandard(CxxStd std);

  /**
   * @brief Gets the C++ standard being compiled for.
   */
  CxxStd getCxxStandard() const { return cxxStandard_; }

  /**
   * @brief Executes the tool and returns a process exit code.
   * @return int 0 for sucess, non zero for error
   */
  int execute() override;

  /**
   * @brief Gets a reference to the hexagon processor configuration.
   *
   * @return HexagonProcConfig&
   */
  HexagonProcConfig &getHexagonProcConfig() { return hexagonProcConfig_; }

  /**
   * @brief If false will instruct the compiler not to expect to link standard
   * libraries and setup the standard runtime environment.
   */
  void setLinkStandardLibsFlag(bool flag) { linkStandardLibs_ = flag; }

private:
  CompilerMode compilerMode_;
  OptLevel optLevel_;
  bool debugFlag_;
  bool saveTemps_;
  std::string sourceFile_;
  std::string outputFile_;
  std::vector<std::string> includes_;
  std::vector<std::string> systemIncludes_;
  std::vector<std::string> additionalCommandLineArgs_;
  std::vector<std::string> defines_;
  std::vector<std::string> warningFlags_;
  HexagonProcConfig hexagonProcConfig_;
  CxxStd cxxStandard_;
  bool linkStandardLibs_;
};
} // namespace qaic
#endif
