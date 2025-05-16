// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Compiler.h"
#include "Tools.h"
#include "support/Debug.h"
#include "support/StringList.h"

#include "llvm/Support/Path.h"

using namespace llvm;

#define DEBUG_TYPE "toolchain"

#define HEXAGON_ARCH_STRING "v68"

qaic::Compiler::Compiler()
    : ToolBase("Compiler"), compilerMode_(Unknown), optLevel_(O1),
      debugFlag_(false), saveTemps_(false), cxxStandard_(cxx_unknown) {}

std::vector<std::string>
qaic::Compiler::HexagonProcConfig::getCommandLine() const {
  std::vector<std::string> args;

  // Default to "mobile" ABI
  args.push_back("--target=hexagon-unknown-elf");

  // Add arch string
  std::string arg = "-m";
  arg += HEXAGON_ARCH_STRING;
  args.push_back(std::move(arg));

  if (HVXEnable) {
    std::string arg = "-mhvx=";
    arg += HEXAGON_ARCH_STRING;
    args.push_back(std::move(arg));
  }

  if (HMXEnable) {
    std::string arg = "-mhmx=";
    arg += HEXAGON_ARCH_STRING;
    args.push_back(arg);
  }

  if (HVXIeeeFpEnable) {
    args.emplace_back("-mhvx-ieee-fp");
  }

  return args;
}

void qaic::Compiler::addIncludePath(llvm::StringRef path) {
  std::string arg = "-I";
  arg += path;
  includes_.push_back(std::move(arg));
  QAIC_DEBUG_STREAM("adding compiler include search path: " << path << "\n");
}

void qaic::Compiler::addDefine(llvm::StringRef def) {
  std::string arg = "-D";
  arg += def;
  defines_.push_back(std::move(arg));
}

void qaic::Compiler::addUndefine(llvm::StringRef def) {
  std::string arg = "-U";
  arg += def;
  defines_.push_back(std::move(arg));
}

void qaic::Compiler::addSystemIncludePath(llvm::StringRef path) {
  std::string arg = "-isystem";
  arg += path;
  includes_.push_back(std::move(arg));
  QAIC_DEBUG_STREAM("adding compiler system include search path: " << path
                                                                   << "\n");
}

void qaic::Compiler::addWarningFlag(llvm::StringRef flag) {
  if (flag.startswith("-W")) {
    warningFlags_.push_back(flag.str());
  } else {
    std::string arg = "-W";
    arg += flag;
    warningFlags_.push_back(arg);
  }
}

void qaic::Compiler::setCxxStandard(CxxStd std) {
  QAIC_DEBUG_STREAM("setting C++ standard to C++" << (int)std << "\n");
  cxxStandard_ = std;
}

int qaic::Compiler::execute() {

  // Check the extension to determine what compiler to use
  // Assume a missing extension is C code
  auto ext = sys::path::extension(sourceFile_).lower();

  std::string compilerPath;
  bool cxx = false;
  if (ext == ".cc" || ext == ".cxx" || ext == ".cpp") {
    QAIC_DEBUG_STREAM("source file type deduction: C++ extension is " << ext
                                                                      << "\n");
    auto pathOrError = getTools().findCXXCompiler();
    if (std::error_code ec = pathOrError.getError()) {
      REPORT_TOOL_ERROR("can't find cxx compiler.\n");
      return ec.value();
    }
    compilerPath = pathOrError.get();
    cxx = true;
  } else if (ext == ".c" || ext == ".s") {
    QAIC_DEBUG_STREAM("source file type deduction: ASM/C extension is "
                      << ext << "\n");
    auto pathOrError = getTools().findCCompiler();
    if (std::error_code ec = pathOrError.getError()) {
      REPORT_TOOL_ERROR("can't find c compiler.\n");
      return ec.value();
    }
    compilerPath = pathOrError.get();
  } else {
    REPORT_TOOL_ERROR("unsupported source file extension '" << ext << "'\n");
    return -1;
  }

  StringList args;
  args.push_back(compilerPath);

  if (sourceFile_.empty()) {
    REPORT_TOOL_ERROR("no source file given.\n");
    return 1;
  }

  if (outputFile_.empty()) {
    REPORT_TOOL_ERROR("no output file given.\n");
    return 1;
  }

  if (compilerMode_ == CompilerMode::PreProcess) {
    args.push_back("-E");
    args.push_back(sourceFile_);
    args.push_back("-o");
    args.push_back(outputFile_);
  } else if (compilerMode_ == CompilerMode::Compile) {
    args.push_back("-c");
    args.push_back(sourceFile_);
    args.push_back("-o");
    args.push_back(outputFile_);
  } else if (compilerMode_ == CompilerMode::CompileNoAssemble) {
    args.push_back("-S");
    args.push_back(sourceFile_);
    args.push_back("-o");
    args.push_back(outputFile_);
  } else {
    REPORT_TOOL_ERROR("mode is unspecified\n");
    return -1;
  }

  std::vector<std::string> procArgs =
      hexagonProcConfig_.getCommandLine();
  for (auto &arg : procArgs) {
    args.push_back(arg);
  }

  for (auto &def : defines_) {
    args.push_back(def);
  }

  for (auto &file : includes_) {
    args.push_back(file);
  }

  if (debugFlag_) {
    args.push_back("-g");
  } else {
    args.push_back("-g0");
  }

  for (auto &f : warningFlags_) {
    args.push_back(f);
  }

  if (saveTemps_) {
    args.push_back("-save-temps");
  }

  if (cxx) {
    switch (cxxStandard_) {
    case cxx11:
      args.push_back("-std=c++11");
      break;
    case cxx14:
      args.push_back("-std=c++14");
      break;
    case cxx17:
      args.push_back("-std=c++17");
      break;
    default:
      break;
    }
  }

  switch (optLevel_) {
  case O0:
    args.push_back("-O0");
    break;
  case O1:
    args.push_back("-O1");
    break;
  case O2:
    args.push_back("-O2");
    break;
  case O3:
    args.push_back("-O3");
    break;
  default:
    break;
  }

  // Force .sdata section to be size zero because the device FW doesn't support
  // .sdata sections
  args.push_back("-G0");

  if (!linkStandardLibs_) {
    // Prevents the compiler from using memcmp, memset, memcpy and memmove even
    // if the code doesn't explicitly call those.
    // Paired with -nostdlib in the linker stage.
    args.push_back("-ffreestanding");
  }

  for (auto &arg : additionalCommandLineArgs_) {
    args.push_back(arg);
  }

  return runProcessWithArgs(compilerPath, args.getRefList());
}
