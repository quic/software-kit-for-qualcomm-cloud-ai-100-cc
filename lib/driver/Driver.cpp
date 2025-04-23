// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Driver.h"
#include "DriverAction.h"
#include "DriverContext.h"
#include "DriverOptions.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"

#include "support/Debug.h"
#include "support/Path.h"

#define DEBUG_TYPE "driver"

#define DRIVER_REPORT_ERROR(X)                                                 \
  do {                                                                         \
    llvm::errs() << "error: " << X;                                            \
  } while (false)

using namespace llvm;
using namespace llvm::opt;
using namespace qaic;

Driver::OutputNames::OutputNames(llvm::StringRef provided)
    : provided_(provided) {}

std::string Driver::OutputNames::getPreProcessStepOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".i";
}

std::string Driver::OutputNames::getCompileStepOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".s";
}

std::string Driver::OutputNames::getAssembleStepOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".o";
}

std::string Driver::OutputNames::getLinkStepOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".elf";
}

std::string Driver::OutputNames::getQPCStepOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".qpc";
}

std::string Driver::OutputNames::getMetadataOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".metadata.bin";
}

std::string Driver::OutputNames::getNetworkDescriptorOutputName() const {
  std::string base, ext;
  std::tie(base, ext) = splitExtension(provided_);
  return base + ".netdesc.bin";
}

static std::string getMainExecutable(llvm::StringRef argv0) {
  void *addr = (void *)(intptr_t)getMainExecutable;
  return sys::fs::getMainExecutable(argv0.data(), addr);
}

const InputArgList &Driver::parseArgs(unsigned &MissingArgIndex,
                                      unsigned &MissingArgCount) {
  auto &optTable = getDriverOptTable();
  MissingArgIndex = MissingArgCount = 0;
  ParsedDriverArgs_ = optTable.ParseArgs(RawDriverArgv_.slice(1),
                                         MissingArgIndex, MissingArgCount);

  // Find the install root from the arguments
  // <install-root>/exec/qaic-cc
  auto exepath = getMainExecutable(RawDriverArgv_[0]);
  installRoot_ = sys::path::parent_path(sys::path::parent_path(exepath)).str();
  installLibPath_ = joinPath(installRoot_, "dev", "lib", "x86_64", "compute");
  installIncludePath_ = joinPath(installRoot_, "dev", "inc", "compute");
  QAIC_DEBUG_STREAM("argv[0] is " << RawDriverArgv_[0] << "\n");
  QAIC_DEBUG_STREAM("QAIC install root: " << installRoot_ << "\n");
  QAIC_DEBUG_STREAM("QAIC lib path: " << installLibPath_ << "\n");
  QAIC_DEBUG_STREAM("QAIC include path: " << installIncludePath_ << "\n");

  return ParsedDriverArgs_;
}

void Driver::printHelp() {
  auto &optTable = getDriverOptTable();
  std::string Usage =
      llvm::formatv("{0} [options] file...", RawDriverArgv_[0]).str();
  optTable.printHelp(llvm::outs(), Usage.c_str(),
                     "qaic-cc QAIC Compiler Driver", false, false,
                     /*ShowAllAliases=*/false);
}

bool Driver::deduceDriverMode() {
  if (ParsedDriverArgs_.size() == 0)
    return false;

  // Figgure out what the user is trying to do?
  if (ParsedDriverArgs_.hasArg(options::OPT_E)) {
    Mode_ = PreProcess;
    return true;
  } else if (ParsedDriverArgs_.hasArg(options::OPT_S)) {
    Mode_ = Compile;
    return true;
  } else if (ParsedDriverArgs_.hasArg(options::OPT_c)) {
    Mode_ = Assemble;
    return true;
  } else if (ParsedDriverArgs_.hasArg(options::OPT_o)) {
    // If we are not loading source code then we are linking or building a QPC
    StringRef outputFile = ParsedDriverArgs_.getLastArgValue(options::OPT_o);
    StringRef ext = sys::path::extension(outputFile);
    if (ext == ".elf" || ext == ".a" || ext == "") {
      if (ParsedDriverArgs_.hasArg(options::OPT_Program_Config)) {
        Mode_ = LinkWithMetadata;
      } else {
        Mode_ = Link; // just build an elf sans metadata
      }
      return true;
    } else if (ext == ".qpc") {
      Mode_ = QPC;
      return true;
    }
  }

  // This really should be an unreachable case
  DRIVER_REPORT_ERROR("can not deduce driver mode.\n");
  return false;
}

bool Driver::run() {

  DriverContext context{*this};

  const char *hexagonToolsDir = std::getenv("HEXAGON_TOOLS_DIR");
  if (hexagonToolsDir == nullptr) {
    llvm::errs() << "WARNING: HEXAGON_TOOLS_DIR is not set!\n";
    llvm::errs() << "qaic-cc may not find the proper Hexagon toolchain.\n";
  } else if (!sys::fs::exists(hexagonToolsDir)) {
    llvm::errs() << "WARNING: HEXAGON_TOOLS_DIR (" << hexagonToolsDir
                 << ") does not exist!\n";
    llvm::errs() << "qaic-cc may not find the proper Hexagon toolchain.\n";
  }

  // Load program config if provided, some action require this while others do
  // not. It is up to the action to check for the required configuration before
  // running.
  if (ParsedDriverArgs_.hasArg(options::OPT_Program_Config)) {

    ProgramConfig cfg;
    auto configFile =
        ParsedDriverArgs_.getLastArgValue(options::OPT_Program_Config);
    if (!cfg.loadFromFile(configFile)) {
      DRIVER_REPORT_ERROR("failed to load program configuration from "
                          << configFile << "\n");
      return false;
    }

    context.setProgram(std::make_unique<ComputeProgram>(std::move(cfg)));
  }

  std::vector<std::unique_ptr<DriverAction>> actionsToRun;

  // Build list of action to perform
  switch (Mode_) {
  case PreProcess:
  case Compile:
  case Assemble:
    // Just execute the compiler to build a .i, .s or .o
    context.addAction(std::make_unique<CompileAction>(*this));
    break;

  case Link:
    // Just execute the linker to build a .a, .elf/exe
    context.addAction(std::make_unique<LinkAction>(*this));
    break;

  case LinkWithMetadata:
    context.addAction(std::make_unique<LinkAction>(*this));
    context.addAction(std::make_unique<ExtractEntryPointAddressAction>(*this));
    context.addAction(std::make_unique<EmbedMetadataAction>(*this));
    break;

  case QPC:
    context.addAction(std::make_unique<LinkAction>(*this));
    context.addAction(std::make_unique<ExtractEntryPointAddressAction>(*this));
    context.addAction(std::make_unique<EmbedMetadataAction>(*this));
    context.addAction(std::make_unique<BuildQPCAction>(*this));
    break;

  default:
    break;
  }

  return context.run();
}
