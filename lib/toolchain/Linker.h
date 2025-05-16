// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_TOOLCHAIN_LINKER_H_
#define _QAIC_TOOLCHAIN_LINKER_H_

#include <map>
#include <string>
#include <vector>

#include "llvm/Support/Error.h"

#include "ToolBase.h"

namespace qaic {

/**
 * @brief Wrapper for the linker tool.
 */
class Linker : public ToolBase {
public:
  static constexpr uintptr_t DEFAULT_TEXT_SECTION_ADDR = 0x2000000;
  static constexpr uintptr_t DEFAULT_STACK_SIZE = 12288; // From NSP Root image
  static const char TEXT_SECTION[];
  static const char START_SECTION[];
  static const char HEXAGON_STACK_SIZE_SYM[];

  explicit Linker();

  /**
   * @brief
   *
   */
  void addInput(llvm::StringRef file) { linkerLibs_.push_back(file.str()); }

  /**
   * @brief
   *
   */
  void addLib(llvm::StringRef lib);

  /**
   * @brief Begins a new linker input group.
   *
   * Input files can be added to the group by calling addInput()
   */
  void startGroup();

  /**
   * @brief Ends the current linker input group.
   */
  void endGroup();

  /**
   * @brief Instructs the linker to not discard unused inputs.
   *
   * Input files can be added by calling addInput()
   */
  void startWholeArchive();

  /**
   * @brief Turn off the effect of startWholeArchive()
   */
  void endWholeArchive();

  /**
   * @brief Adds a search path to the linker for locating input libraries.
   */
  void addLinkerSearchPath(llvm::StringRef path);

  /**
   * @brief Sets the path of the output file.
   */
  void setOutputFile(llvm::StringRef file) { outputFile_ = file; }

  /**
   * @brief Gets the path of the output file.
   */
  llvm::StringRef getOutputFile() const { return outputFile_; }

  /**
   * @brief Explicitly set the address of an ELF section.
   *
   * @param sectionName The name of the ELF section.
   * @param addr The address to set
   */
  void setSectionAddr(llvm::StringRef sectionName, uintptr_t addr) {
    sectionAddrs_[sectionName.str()] = addr;
  }

  /**
   * @brief Gets the address of an ELF section set by setSectionAddr().
   *
   * @param sectionName The name of the ELF section.
   * @return uintptr_t Address or 0 if not set.
   */
  uintptr_t getSectionAddr(llvm::StringRef sectionName) const;

  /**
   * @brief Defines a new ELF symbol.
   *
   * @param sym The name of the symbol.
   * @param expr An expression to compute the symbol value.
   */
  void defineSymbol(llvm::StringRef sym, llvm::StringRef expr) {
    defSyms_[sym.str()] = expr;
  }

  /**
   * @brief Defines a new ELF symbol.
   *
   * @param sym The name of the symbol.
   * @param expr The symbol value.
   */
  void defineSymbol(llvm::StringRef sym, uint64_t value) {
    defineSymbol(sym, std::to_string(value));
  }

  /**
   * @brief If true will instruct the linker to link standard libraries and
   * setup the
   * standard runtime environment.
   */
  void setLinkStandardLibsFlag(bool flag) { linkStandardLibs_ = flag; }

  /**
   * @brief Returns true if the link standard libraries flag is set.
   */
  bool getLinkStandardLibsFlag() const { return linkStandardLibs_; }

  /**
   * @brief If true will instruct the linker to output verbose linking
   * information.
   */
  void setVerbose(bool flag) { verbose_ = flag; }

  /**
   * @brief Returns true if the verbose linking flag is set.
   */
  bool getVerbose() const { return verbose_; }

  void addPretendUndef(llvm::StringRef sym) {
    pretendUndefSyms_.push_back(sym.str());
  }

  void wrapSym(llvm::StringRef sym) { wrapSyms_.push_back(sym.str()); }

  /**
   * @brief Executes the tool and returns a process exit code.
   * @return int 0 for sucess, non zero for error
   */
  int execute() override;

private:
  bool group_;
  bool wholeArchive_;
  std::vector<std::string> linkerLibs_;
  std::string outputFile_;
  std::map<std::string, uintptr_t> sectionAddrs_;
  std::map<std::string, std::string> defSyms_;
  std::vector<std::string> pretendUndefSyms_;
  std::vector<std::string> wrapSyms_;
  bool linkStandardLibs_;
  bool verbose_;
};
} // namespace qaic

#endif
