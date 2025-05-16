// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <fstream>

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"

#include "Driver.h"
#include "DriverAction.h"
#include "DriverContext.h"

#include "program/QPCBuilder.h"
#include "support/Debug.h"
#include "support/Path.h"

using namespace qaic;
using namespace llvm;
using namespace llvm::object;

#define DRIVER_ACTION_REPORT_ERROR(X)                                          \
  do {                                                                         \
    llvm::errs() << "error: " << X;                                            \
  } while (false)

#define DEBUG_TYPE "Driver"

DriverAction::DriverAction(Driver &D, llvm::StringRef name)
    : D_(D), actionName_(name.str()) {}

const llvm::opt::InputArgList &DriverAction::getDriverArgs() const {
  return D_.getParsedArgs();
}

bool DriverAction::run(DriverContext &context) {
  bool precheck;
  QAIC_DEBUG_STREAM("[Pre-check " << getActionName() << "]\n");
  precheck = preRunCheckImpl(context);
  QAIC_DEBUG_STREAM("[Pre-check " << getActionName() << " "
                                  << (precheck ? "OK" : "ERROR") << "]\n");

  if (!precheck)
    return false;

  bool run;
  QAIC_DEBUG_STREAM("[Run " << getActionName() << "]\n");
  run = runImpl(context);
  QAIC_DEBUG_STREAM("[Run " << getActionName() << " " << (run ? "OK" : "ERROR")
                            << "]\n");

  return run;
}

CompileAction::CompileAction(Driver &D)
    : DriverAction(D, "CompileAction"), compiler_() {}

bool CompileAction::preRunCheckImpl(DriverContext &context) const {

  bool compileFlag = false;
  compileFlag |= getDriverArgs().hasArg(options::OPT_c);
  compileFlag |= getDriverArgs().hasArg(options::OPT_E);
  compileFlag |= getDriverArgs().hasArg(options::OPT_S);

  assert(compileFlag && "missing expected flags");
  if (!compileFlag) {
    return false;
  }

  return true;
}

bool CompileAction::runImpl(DriverContext &context) {
  compiler_.getTools().loadStandardEnvPaths();

  compiler_.setOutputFile(getDriverArgs().getLastArgValue(options::OPT_o));

  if (getDriverArgs().hasArg(options::OPT_c)) {
    compiler_.setSourceFile(
        getDriverArgs().getLastArgValue(options::OPT_INPUT));
    compiler_.setMode(Compiler::Compile);
  } else if (getDriverArgs().hasArg(options::OPT_E)) {
    compiler_.setSourceFile(
        getDriverArgs().getLastArgValue(options::OPT_INPUT));
    compiler_.setMode(Compiler::PreProcess);
  } else if (getDriverArgs().hasArg(options::OPT_S)) {
    compiler_.setSourceFile(
        getDriverArgs().getLastArgValue(options::OPT_INPUT));
    compiler_.setMode(Compiler::CompileNoAssemble);
  } else {
    llvm_unreachable("");
    return false;
  }

  for (auto &I : getDriverArgs().getAllArgValues(options::OPT_I)) {
    compiler_.addIncludePath(I);
  }

  for (auto &I : getDriverArgs().getAllArgValues(options::OPT_isystem)) {
    compiler_.addSystemIncludePath(I);
  }

  for (auto &W : getDriverArgs().getAllArgValues(options::OPT_W_Joined)) {
    compiler_.addWarningFlag(W);
  }

  // Add additional paths for the minimal runtime headers (e.g.
  // AICMetadataExecCtx.h)
  auto runtimeHeaderPath = (getDriver().getInstallIncludePath()).str();
  compiler_.addIncludePath(runtimeHeaderPath);

  // Add -D and -U preserving order
  for (auto A : getDriverArgs()) {
    if (A->getOption().matches(options::OPT_D)) {
      compiler_.addDefine(A->getValue());
    } else if (A->getOption().matches(options::OPT_U)) {
      compiler_.addDefine(A->getValue());
    }
  }

  // Set optimization / debug flag
  bool has_g0 = false;
  for (auto A : getDriverArgs()) {
    if (A->getOption().matches(options::OPT_O0)) {
      compiler_.setOptLevel(Compiler::O0);
    } else if (A->getOption().matches(options::OPT_O1)) {
      compiler_.setOptLevel(Compiler::O1);
    } else if (A->getOption().matches(options::OPT_O2)) {
      compiler_.setOptLevel(Compiler::O2);
    } else if (A->getOption().matches(options::OPT_O3)) {
      compiler_.setOptLevel(Compiler::O3);
    } else if (A->getOption().matches(options::OPT_g)) {
      compiler_.setDebugFlag(true);
    } else if (A->getOption().matches(options::OPT_g0)) {
      has_g0 = true;
    }
  }

  // If -g0 is given disable debug regardless of order
  if (has_g0) {
    compiler_.setDebugFlag(false);
  }

  auto &hexagonCgf = compiler_.getHexagonProcConfig();
  hexagonCgf.HMXEnable = false;
  hexagonCgf.HVXEnable = true;
  hexagonCgf.HVXIeeeFpEnable = true;

  if (getDriverArgs().hasArg(options::OPT_stdcxx)) {
    auto value = getDriverArgs().getLastArgValue(options::OPT_stdcxx);
    auto cxxstd = StringSwitch<Compiler::CxxStd>(value)
                      .Case("c++11", Compiler::cxx11)
                      .Case("c++14", Compiler::cxx14)
                      .Case("c++17", Compiler::cxx17)
                      .Default(Compiler::cxx_unknown);
    if (cxxstd != Compiler::cxx_unknown) {
      compiler_.setCxxStandard(cxxstd);
    } else {
      DRIVER_ACTION_REPORT_ERROR("unknown c++ standard '" << value << "'\n");
      return true;
    }
  }

  compiler_.setSaveTempsFlag(getDriverArgs().hasArg(options::OPT_save_temps));

  compiler_.setLinkStandardLibsFlag(
      getDriverArgs().hasArg(options::OPT_WithStdLibraries));

  return (compiler_.execute() == 0);
}

LinkAction::LinkAction(Driver &D)
    : DriverAction(D, "LinkAction"), linker_() {}

bool LinkAction::runImpl(DriverContext &context) {

  // Load standard environment paths for tools
  linker_.getTools().loadStandardEnvPaths();

  linker_.setLinkStandardLibsFlag(
      getDriverArgs().hasArg(options::OPT_WithStdLibraries));

  // Compute the output file name - in link only mode we use whatever the
  // command line has directly.
  Driver::OutputNames outputNames{
      getDriverArgs().getLastArgValue(options::OPT_o)};
  if (getDriver().isLinkOnlyMode()) {
    linker_.setOutputFile(outputNames.getProvidedName());
  } else {
    linker_.setOutputFile(outputNames.getLinkStepOutputName());
  }

  // Add install library path to linker search paths
  if (!getDriverArgs().hasArg(options::OPT_NoDefaultLibPaths)) {
    linker_.addLinkerSearchPath(getDriver().getInstallLibPath());
  }

  // Add provided library search paths
  for (auto &s : getDriverArgs().getAllArgValues(options::OPT_L)) {
    linker_.addLinkerSearchPath(s);
  }

  // In debug mode always run the linker in verbose mode
  QAIC_DEBUG(linker_.setVerbose(true));

  for (auto &s : getDriverArgs().getAllArgValues(options::OPT_u)) {
    linker_.addPretendUndef(s);
  }

  // Parse other args in order specified on the command line

  for (auto A : getDriverArgs()) {
    // Handle linker directives
    if (A->getOption().matches(options::OPT_Wl_comma)) {
      if (A->containsValue("--whole-archive")) {
        linker_.startWholeArchive();
      } else if (A->containsValue("--no-whole-archive")) {
        linker_.endWholeArchive();
      } else if (A->containsValue("--start-group")) {
        linker_.startGroup();
      } else if (A->containsValue("--end-group")) {
        linker_.endGroup();
      } else if (A->containsValue("--verbose")) {
        linker_.setVerbose(true);
      } else if (A->containsValue("--wrap")) {
        for (unsigned int i = 1; i < A->getNumValues(); i++) {
          linker_.wrapSym(A->getValue(i));
        }
      } else {
        DRIVER_ACTION_REPORT_ERROR("Unsupported linker argument "
                                   << A->getValue() << "\n");
        return false;
      }
      // Handle input objects
    } else if (A->getOption().matches(options::OPT_INPUT)) {
      for (auto &s : A->getValues()) {
        linker_.addInput(s);
      }
      // Handle libraries
    } else if (A->getOption().matches(options::OPT_l)) {
      for (auto &s : A->getValues()) {
        linker_.addLib(s);
      }
    }
  }

  // Link the runtime, and make sure we pull in the start symbol
  linker_.addPretendUndef("_qaic_start");
  linker_.startGroup();
  linker_.addInput("-lqaicrt");
  linker_.addInput("-ldevRuntime");
  linker_.endGroup();


  // Setup default sections/stack information
  if (linker_.getLinkStandardLibsFlag()) {
    linker_.setSectionAddr(Linker::START_SECTION,
                            Linker::DEFAULT_TEXT_SECTION_ADDR);
  } else {
    linker_.setSectionAddr(Linker::TEXT_SECTION,
                            Linker::DEFAULT_TEXT_SECTION_ADDR);
    linker_.defineSymbol(Linker::HEXAGON_STACK_SIZE_SYM,
                          Linker::DEFAULT_STACK_SIZE);
  }

  return (linker_.execute() == 0);
}

EmbedMetadataAction::EmbedMetadataAction(Driver &D)
    : DriverAction(D, "EmbedMetadataAction"), objcopy_() {}

bool EmbedMetadataAction::preRunCheckImpl(DriverContext &context) const {
  assert(context.getProgram() != nullptr &&
         "missing program context for action");
  if (!context.getProgram()) {
    DRIVER_ACTION_REPORT_ERROR("missing program context for action.");
    return false;
  }
  return true;
}

bool EmbedMetadataAction::runImpl(DriverContext &context) {

  if (context.getProgram() == nullptr) {
    assert(false && "missing program context for action.");
    return false;
  }

  // Compute the output file name - in link only mode we use whatever the
  // command line has directly.
  Driver::OutputNames outputNames{
      getDriverArgs().getLastArgValue(options::OPT_o)};
  std::string inputFileName;
  if (getDriver().isLinkOnlyMode()) {
    inputFileName = outputNames.getProvidedName();
  } else {
    // use the output filename from the previous step (linking)
    inputFileName = outputNames.getLinkStepOutputName();
  }

  // Generate and write the metadata to a file
  auto metadataPtr = context.getProgram()->generateMetadata();
  auto metadataFileName = outputNames.getMetadataOutputName();
  metadataPtr->writeMetadata(metadataFileName.c_str());

  // Run objcopy to embed the metadata
  objcopy_.getTools().loadStandardEnvPaths();
  objcopy_.setInputFile(inputFileName);
  objcopy_.addSection("metadata", metadataFileName);
  objcopy_.setSectionFlag("metadata", ObjCopy::noload);

  int rc = (objcopy_.execute() == 0);

  // Delete the temporary metadata file
  if (!getDriverArgs().hasArg(options::OPT_save_temps)) {
    auto ec = sys::fs::remove(metadataFileName);
    if (ec) {
      DRIVER_ACTION_REPORT_ERROR("Failed to remove temp build file "
                                 << metadataFileName << "\n");
      return ec.value();
    }
  }

  return rc;
}

ExtractEntryPointAddressAction::ExtractEntryPointAddressAction(Driver &D)
    : DriverAction(D, "ExtractEntryPointAddressAction") {}

bool ExtractEntryPointAddressAction::preRunCheckImpl(
    DriverContext &context) const {
  assert(context.getProgram() != nullptr &&
         "missing program context for action");
  if (!context.getProgram()) {
    DRIVER_ACTION_REPORT_ERROR("missing program context for action.");
    return false;
  }
  return true;
}

bool ExtractEntryPointAddressAction::runImpl(DriverContext &context) {

  // Compute the output file name - in link only mode we use whatever the
  // command line has directly.
  Driver::OutputNames outputNames{
      getDriverArgs().getLastArgValue(options::OPT_o)};
  std::string inputFileName;
  if (getDriver().isLinkOnlyMode()) {
    inputFileName = outputNames.getProvidedName();
  } else {
    // use the output filename from the previous step (linking)
    inputFileName = outputNames.getLinkStepOutputName();
  }

  auto BufferOrErr = MemoryBuffer::getFileOrSTDIN(inputFileName);
  if (!BufferOrErr) {
    llvm::dbgs() << "Failed to load ELF " << inputFileName << "\n";
    return false;
  }

  auto BinaryOrErr =
      createBinary(BufferOrErr.get()->getMemBufferRef(), nullptr);
  if (!BinaryOrErr) {
    llvm::errs() << "Failed to create binary object.\n";
    return false;
  }

  Binary &Bin = *BinaryOrErr.get();
  ObjectFile *Object = llvm::dyn_cast<ObjectFile>(&Bin);
  assert(Object != nullptr && "unexpected file type");
  if (!Object) {
    return false;
  }

  // Find the _qaic_start symbol within the elf
  llvm::Optional<uint64_t> activateAddr;
  for (SymbolRef Sym : Object->symbols()) {
    auto type = Sym.getType();
    if (!type || type.get() != SymbolRef::ST_Function)
      continue;
    auto name = Sym.getName();
    if (!name || name.get() != "_qaic_start")
      continue;
    auto addr = Sym.getAddress();
    if (addr) {
      activateAddr = addr.get();
      break;
    }
  }

  context.getProgram()->setEntrypointAddr(activateAddr.getValue());
  QAIC_DEBUG_STREAM("found activate function at " << activateAddr.getValue()
                                                  << "\n");

  return activateAddr.hasValue();
};

BuildQPCAction::BuildQPCAction(Driver &D) : DriverAction(D, "BuildQPCAction") {}

bool BuildQPCAction::preRunCheckImpl(DriverContext &context) const {
  assert(context.getProgram() != nullptr &&
         "missing program context for action");
  if (!context.getProgram()) {
    DRIVER_ACTION_REPORT_ERROR("missing program context for action.");
    return false;
  }
  return true;
}

bool BuildQPCAction::runImpl(DriverContext &context) {

  // The input name must be a .elf from the previous step
  // The output name can be whatever the user provided
  Driver::OutputNames outputNames{
      getDriverArgs().getLastArgValue(options::OPT_o)};
  auto inputName = outputNames.getLinkStepOutputName();
  auto outputName = outputNames.getProvidedName();

  QPCBuilder builder;

  // Add the elf + metadata
  if (!builder.addSegmentFromFile("network.elf", inputName)) {
    DRIVER_ACTION_REPORT_ERROR("failed to add " << inputName << " to QPC.\n");
    return false;
  }

  // Add the constants
  std::vector<std::string> constfiles = {"constants.bin", "constantsdesc.bin"};
  for (auto const &file : constfiles) {
    if (!builder.addSegmentFromFile(file, file)) {
      DRIVER_ACTION_REPORT_ERROR("failed to add " << file << " to QPC.\n");
      return false;
    }
    // Delete the temporary file
    if (!getDriverArgs().hasArg(options::OPT_save_temps)) {
      auto ec = sys::fs::remove(file);
      if (ec) {
        DRIVER_ACTION_REPORT_ERROR("Failed to remove temp build file " << file
                                                                       << "\n");
        return ec.value();
      }
    }
  }

  // Serialize and add network descriptor
  std::string networkDescBuffer;
  {
    auto networkDesc = context.getProgram()->generateNetworkDescriptor();
    if (!networkDesc->SerializeToString(&networkDescBuffer)) {
      DRIVER_ACTION_REPORT_ERROR("failed to serialize network descriptor.\n");
      return false;
    }

    builder.addSegment("networkdesc.bin",
                       (const uint8_t *)networkDescBuffer.c_str(),
                       networkDescBuffer.size(), 0);
  }

  auto serializedQPC = builder.finalizeToByteArray();
  {
    std::ofstream file(outputName.str(), std::ios::binary);
    file.write((const char *)serializedQPC.data(), serializedQPC.size());
  }

  if (getDriverArgs().hasArg(options::OPT_save_temps)) {
    std::ofstream file(outputNames.getNetworkDescriptorOutputName(),
                       std::ios::binary);
    file.write((const char *)networkDescBuffer.data(),
               networkDescBuffer.size());
  }

  return true;
};
