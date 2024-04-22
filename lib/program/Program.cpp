// Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <bitset>
#include <set>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"

#include "../../runtime/lib/AICDefsInternal.h"
#include "Program.h"
#include "ProgramDesc.h"
#include "networkdesc/qpc/inc/QAicQpc.h"
#include "llvm/Support/FormatVariadic.h"
#include <google/protobuf/util/json_util.h>

using namespace llvm;
using namespace qaic;

const uint32_t VTCM_MAX_SIZE = 8 * 1024 * 1024;  /*8MB VTCM*/
const uint32_t L2TCM_MAX_SIZE = 1 * 1024 * 1024; /*1MB L2TCM*/
const uint32_t COMPUTE_MAX_SEMAPHORES = 2;
const uint32_t DB_SIZE = 4; /*Bytes per doorbell*/

ComputeProgram::ComputeProgram(ProgramConfig &&config)
    : config_(std::move(config)), entryPoint_(0) {}

static AICMDMulticastEntryAddrSpace getMCSpace(aicnwdesc::destination ioType) {
  if (aicnwdesc::L2TCM == ioType) {
    return AICMDAddrSpaceL2TCM;
  } else if (aicnwdesc::VTCM == ioType) {
    return AICMDAddrSpaceVTCM;
  } else {
    llvm::errs() << "Invalid multicast destination: " << std::to_string(ioType)
                 << "\n";
    exit(-1);
  }
}

static AICMDDMAEntryAddrSpace getDMASpace(aicnwdesc::destination ioType) {
  if (aicnwdesc::DDR == ioType) {
    return AICMDDMAAddrSpaceDDR;
  } else {
    return AICMDDMAAddrSpaceMC;
  }
}

static bool isMC(aicnwdesc::destination space) {
  return aicnwdesc::DDR != space;
}

static uint32_t getNspMask(const aicnwdesc::IODescription &buff,
                           uint32_t numNsps) {
  uint32_t allNspsMask = (0x1U << numNsps) - 1;
  uint32_t nspMask = 0;
  if (!buff.nsps_size())
    return allNspsMask;
  for (auto nspNum : buff.nsps()) {
    if (nspNum >= numNsps) {
      llvm::errs() << "Config Error: Invalid nsp number " +
                          std::to_string(nspNum) + ". Valid values are 0-" +
                          std::to_string(numNsps - 1) + "\n";
      exit(-1);
    }
    nspMask |= (0x1U << nspNum);
  }
  return nspMask;
}

static void getNumThreads(uint32_t &numThreads, uint32_t &numHmxThreads,
                          uint32_t &numHvxThreads,
                          const aicnwdesc::ProgramConfig &nm_proto) {
  numThreads = nm_proto.numthreads();
  numHmxThreads = nm_proto.numhmxthreads();
  if (numThreads == 0) {
    numThreads =
        4 + numHmxThreads; // Default to 4 HVX threads if none are specified
  }
  assert(numHmxThreads <= numThreads);
  numHvxThreads = numThreads - numHmxThreads;
  if (numThreads > aic::MAX_NUM_THREADS) {
    llvm::errs()
        << "Config Error: numThreads isn't valid. Supported values are: 1-6\n";
    exit(-1);
  }
}

using AICFlatbufMDPortType = AicMetadataFlat::AICMDPortType;
static AICFlatbufMDPortType getPortType(AicMetadataFlat::AICMDPortType portInfoType) {
  switch (portInfoType) {
  default:
  llvm:
    errs() << "Unexpected port type.n";
  case AICFlatbufMDPortType::AICMDPortType_AICMDPortUserIO:
    return AICFlatbufMDPortType::AICMDPortType_AICMDPortUserIO;
  }
}


std::unique_ptr<MetadataFlatbufferWriter> ComputeProgram::generateMetadata() const {
  auto &nm_proto = config_.get();
  if (nm_proto.hwversionmajor() != 2) {
    llvm::errs() << "Config Error: hwVersionMajor isn't valid. Supported "
                    "values are: 2\n";
    exit(-1);
  }
  if (nm_proto.hwversionminor() != 0) {
    llvm::errs() << "Config Error: hwVersionMinor isn't valid. Supported "
                    "values are: 0\n";
    exit(-1);
  }
  std::unique_ptr<MetadataFlatbufferWriter> metadata{new MetadataFlatbufferWriter(
      nm_proto.hwversionmajor(), nm_proto.hwversionminor())};

  uint16_t numNsps = nm_proto.numnsps();
  if (numNsps < 1 || numNsps > aic::MAX_NUM_CORES) {
    llvm::errs()
        << "Config Error: numNSPs isn't valid. Supported values are: 1-16\n";
    exit(-1);
  }
  uint32_t allNspsMask = (0x1U << numNsps) - 1;

  metadata->setNetworkName(nm_proto.name());

  // Number of NSPs
  metadata->setNumNSPs(numNsps);

  // Number of semaphores
  metadata->setNumSemaphores(COMPUTE_MAX_SEMAPHORES);

  DoorbellOps doorbellOps_in, doorbellOps_out;
  SemaphoreOps semaphoreOps_in, semaphoreOps_out;

  uint16_t fileNum = 0;
  uint64_t hostOffset = 0;
  uint8_t dynamic_entry = 0;
  const uint16_t inputSem = 0;
  const uint16_t outputSem = 1;

  size_t ddrBuffersSize = 0;
  size_t l2tcmBuffersSize = 1; // AICMetadataWriter won't allow 0
  size_t vtcmBuffersSize = 1;  // AICMetadataWriter won't allow 0
  uint16_t input_port_id = 100;
  uint16_t output_port_id = 101;

  uint32_t numThreads, numHmxThreads, numHvxThreads;
  getNumThreads(numThreads, numHmxThreads, numHvxThreads, nm_proto);

  // MC ID 0 is for L2TCM doorbells
  // DBNum is for input buffer DBs, then output buffer DBs
  // MC ID 1:N is for input buffers to VTCM/L2TCM
  // MC ID N+1:M is for output buffers to VTCM/L2TCM
  // MC ID is a "don't care" for DDR buffers

  // Create MC ID 0, for broadcasting DB to all NSPs except self
  // DBs start at address 0 in L2TCM
  uint16_t mcId = 0;
  uint64_t DBNum = 0;
  uint32_t DBData = 1;
  int numDBs = 1; // Always have exit DB
  for (auto const &buff : nm_proto.inputs()) {
    if (!buff.nodoorbell())
      numDBs++;
  }
  for (auto const &buff : nm_proto.outputs()) {
    if (!buff.nodoorbell())
      numDBs++;
  }
  for (auto const &buff : nm_proto.internalbuffers()) {
    if (!buff.nodoorbell())
      numDBs++;
  }
  if (numDBs < 281) {
    numDBs = 281;
  } else {
    assert(numDBs < 281 &&
           "Can only have 280 buffer doorbells due to hardcoded FW");
  }
  const unsigned int DBSpaceSize = (numDBs * DB_SIZE);
  metadata->addHostMulticastEntry(allNspsMask, DBSpaceSize);
  for (uint16_t core = 0; core < numNsps; ++core) {
    metadata->addNSPMulticastEntry(core, dynamic_entry,
                                   /*nspMask*/ allNspsMask & ~(0x1U << core),
                                   DBSpaceSize, AICMDAddrSpaceL2TCM,
                                   /*offset*/ 0);
  }

  // Initial size for L2TCM inits
  // L2TCM will contain the following, in order:
  //  - DBs per buffer
  //  - exit DB
  //  - udma dummy descriptor
  //  - udma descriptors (8 per thread?)
  //  - user data
  uint32_t udmaDummyStartDescOffset = alignTo(DBSpaceSize, CACHE_LINE_SIZE);
  uint32_t udmaBufferStartOffset = alignTo(
      udmaDummyStartDescOffset + sizeof(aic::DMADescriptor), CACHE_LINE_SIZE);
  uint32_t udmaBufferSize =
      numThreads * CACHE_LINE_SIZE * NUM_UDMA_CACHELINES_PER_THREAD;

  metadata->initL2TCMResize(udmaDummyStartDescOffset +
                            sizeof(aic::DMADescriptor));

  size_t reservedL2TCMSize = udmaDummyStartDescOffset + udmaBufferSize;
  l2tcmBuffersSize = std::max(l2tcmBuffersSize, reservedL2TCMSize);

  // Initialize the dummy Descriptor to done
  aic::DMADescriptor dummyDesc{};
  memset(&dummyDesc, 0x0, sizeof(aic::DMADescriptor));
  dummyDesc.done = 1;
  for (uint64_t i = 0; i < sizeof(aic::DMADescriptor) / sizeof(uint32_t);
       i++) {
    metadata->initL2TCMWord(udmaDummyStartDescOffset + i * sizeof(uint32_t),
                            ((uint32_t *)&dummyDesc)[i]);
  }

  /* Adding port id, port type for DMA request */
  metadata->addPort(input_port_id, getPortType(AicMetadataFlat::AICMDPortType_AICMDPortUserIO));
  metadata->addPort(output_port_id, getPortType(AicMetadataFlat::AICMDPortType_AICMDPortUserIO));

  // Exit DB is the last 4 bytes of DB space
  // Current FW hardcodes it to 0x460
  metadata->setExitDoorbellOffset(DBSpaceSize - DB_SIZE);
  metadata->initL2TCMWord(DBSpaceSize - DB_SIZE, 0x0);

  mcId++;

  ProgramDesc progDesc(/*exitDB*/ numDBs - 1, inputSem, outputSem, numThreads);
  assert(numDBs - 1 == 280);

  auto processBuff = [&](const aicnwdesc::IODescription &buff,
                         usageType_t usage, uint16_t semNum,
                         uint16_t semWaitVal, uint16_t semInitVal, bool last) {
    SemaphoreOps semaphoreOps;
    DoorbellOps doorbellOps;

    uint32_t dmaSize = getIOSize(buff);
    aicnwdesc::destination memType = buff.dest();
    // * baseAddrOffset is how far into the L2TCM/VTCM the MC group should
    // start (must be multiple of 4k). It gets added to the devOffset
    // * devOffset is how far into the sharedDDR or the MC group the
    // transfer should start
    // * Final addr is base(memtype) + baseAddrOffset(mcid) + devOffset
    uint64_t devOffset = buff.devoffset();
    uint64_t baseAddrOffset = buff.baseaddroffset();
    uint32_t nspMask = getNspMask(buff, numNsps);

    // Buffer MC entry (if necessary)
    if (isMC(memType)) {
      if ((baseAddrOffset & ((1U << 12) - 1)) != 0) {
        llvm::errs() << "Config Error: baseAddrOffset values must be multiples "
                        "of 4096\n";
        exit(-1);
      }
      if (usage == USAGE_OUTPUT && numNsps != 1 &&
          ((nspMask & (nspMask - 1)) != 0)) {
        llvm::errs() << "Config Error: Each outputs buffer in L2TCM/VTCM must "
                        "be limited to a single NSP\n";
        exit(-1);
      }
      if (usage != USAGE_INTERNAL) {
        metadata->addHostMulticastEntry(nspMask, dmaSize);
      }
      for (uint16_t core = 0; core < numNsps; ++core) {
        metadata->addNSPMulticastEntry(
            core, dynamic_entry, (usage == USAGE_INTERNAL) ? nspMask : 0,
            dmaSize, getMCSpace(memType), baseAddrOffset);
      }
      mcId++;
      if (memType == aicnwdesc::L2TCM) {
        l2tcmBuffersSize =
            std::max(l2tcmBuffersSize, baseAddrOffset + devOffset + dmaSize);
        if (l2tcmBuffersSize > L2TCM_MAX_SIZE) {
          llvm::errs() << "Config Error: L2TCM buffers can't go past " +
                              std::to_string(L2TCM_MAX_SIZE) + "\n";
          exit(-1);
        }
        if (devOffset + baseAddrOffset < reservedL2TCMSize) {
          llvm::errs()
              << "Config Error: L2TCM buffers can't start before offset " +
                     std::to_string(reservedL2TCMSize) + " when running with " +
                     std::to_string(numThreads) + " threads\n";
          exit(-1);
        }
      } else {
        vtcmBuffersSize =
            std::max(vtcmBuffersSize, baseAddrOffset + devOffset + dmaSize);
        if (vtcmBuffersSize > VTCM_MAX_SIZE) {
          llvm::errs() << "Config Error: VTCM buffers can't go past " +
                              std::to_string(VTCM_MAX_SIZE) + "\n";
          exit(-1);
        }
      }
    } else {
      if (baseAddrOffset != 0) {
        llvm::errs()
            << "Config Error: DDR buffers don't support baseAddrOffest\n";
        exit(-1);
      }
      ddrBuffersSize = std::max(ddrBuffersSize, devOffset + dmaSize);
    }

    if (usage != USAGE_INTERNAL) {
      if (buff.insyncfence() != 0 && buff.insyncfence() != 1) {
        llvm::errs() << "Config Error: inSyncFence must be 0 or 1\n";
        exit(-1);
      }
      if (buff.outsyncfence() != 0 && buff.outsyncfence() != 1) {
        llvm::errs() << "Config Error: outSyncFence must be 0 or 1\n";
        exit(-1);
      }
      // I/O Semaphore
      metadata->addSemaphoreOp(semaphoreOps, AICMDSemaphoreCmdWAITEQ, semNum,
                               semWaitVal, AICMDSemaphoreSyncPre,
                               buff.insyncfence(), buff.outsyncfence());

      if (last) {
        metadata->addSemaphoreOp(semaphoreOps, AICMDSemaphoreCmdINIT, semNum,
                                 semInitVal, AICMDSemaphoreSyncPost,
                                 buff.insyncfence(), buff.outsyncfence());
      }
    }

    // I/O DB
    uint64_t DBOffset = DBNum * DB_SIZE;
    if (usage != USAGE_INTERNAL) {
      metadata->addDoorbellOp(doorbellOps, AICMDDoorballOpSize32, /*mcId*/ 0,
                              DBOffset, DBData);
    }

    if (usage == USAGE_INPUT) {
      metadata->initL2TCMWord(DBOffset, 0);
    } else if (buff.allowpartial()) {
      llvm::errs() << "Only Input buffers can have allowPartial set\n";
      exit(-1);
    } else if (usage == USAGE_OUTPUT) {
      metadata->initL2TCMWord(DBOffset, DBData);
    } else if (usage == USAGE_INTERNAL) {
      metadata->initL2TCMWord(DBOffset, 0);
    } else {
      llvm::errs() << "Only Input, Output, and Internal buffers are currently "
                      "supported\n";
      exit(-1);
    }

    progDesc.addBuffer(buff,
                       /*waitDBNum*/ DBNum,
                       /*ioDBNum*/ 0,
                       /*waitDBVal*/ DBData,
                       /*ioDBVal*/ DBData,
                       /*ioMCID*/ 0,
                       /*ioDBMCID*/ 0,
                       /*buffMCID*/ isMC(memType) ? mcId - 1 : 0,
                       /*nspMask*/ nspMask,
                       /*usage*/ usage,
                       /*allowPartial*/ buff.allowpartial());
    DBNum++;

    if (usage != USAGE_INTERNAL) {
      // I/O DMA
      AICMDDMADirection dir = (usage == USAGE_INPUT) ? AICMDDMAIn : AICMDDMAOut;
      metadata->addDMARequest(fileNum, hostOffset, getDMASpace(memType),
                              devOffset, dmaSize, dir,
                              (usage == USAGE_INPUT) ? input_port_id : output_port_id,
                              (isMC(memType) ? mcId - 1 : 0),
                              semaphoreOps, doorbellOps, AicMetadataFlat::AICMDDMAReserved_AICMDDMATransactionIdNone);
      fileNum++;
    }
  };

  uint16_t inMask = 0;
  uint16_t outMask = 0;
  for (auto const &buff : nm_proto.inputs()) {
    inMask |= getNspMask(buff, numNsps);
  }
  for (auto const &buff : nm_proto.outputs()) {
    outMask |= getNspMask(buff, numNsps);
  }

  // Input buffers
  uint16_t semNum = inputSem;
  uint16_t semInitVal = 0;
  uint16_t semWaitVal = std::bitset<aic::MAX_NUM_CORES>(inMask).count();
  int lastIndex = nm_proto.inputs_size() - 1;
  metadata->initSemaphore(semNum, semInitVal);
  for (int i = 0; i < nm_proto.inputs_size(); i++)
    processBuff(nm_proto.inputs(i), USAGE_INPUT, semNum, semWaitVal, semInitVal,
                i == lastIndex);

  // Output buffers
  semNum = outputSem;
  semInitVal = std::bitset<aic::MAX_NUM_CORES>(outMask).count();
  semWaitVal = 0;
  lastIndex = nm_proto.outputs_size() - 1;
  metadata->initSemaphore(semNum, semInitVal);
  for (int i = 0; i < nm_proto.outputs_size(); i++)
    processBuff(nm_proto.outputs(i), USAGE_OUTPUT, semNum, semWaitVal,
                semInitVal, i == lastIndex);

  // Internal buffers
  for (auto const &buff : nm_proto.internalbuffers())
    processBuff(buff, USAGE_INTERNAL, /*semNum*/ 0, /*semWaitVal*/ 0,
                /*semInitVal*/ 0, /*lastIndex*/ false);

  // UDMA descriptors buffer
  aicnwdesc::IODescription udmaBuff;
  ::google::protobuf::util::JsonParseOptions options;
  std::string udmaBufferDescStr =
      llvm::formatv("{\"type\":\"Int8Ty\",\"dims\":{0},\"hostOffset\":0,"
                    "\"devOffset\":{1},\"baseAddrOffset\":0,\"dest\":\"L2TCM\","
                    "\"inSyncFence\":0,\"outSyncFence\":0}",
                    udmaBufferSize, udmaBufferStartOffset);
  ::google::protobuf::StringPiece strPiece{udmaBufferDescStr.data()};
  auto status = ::google::protobuf::util::JsonStringToMessage(
      strPiece, &udmaBuff, options);
  if (!status.ok()) {
    llvm::errs() << "Json Parsing Failed: " << status.ToString() << "\n";
    exit(-1);
  }
  progDesc.addUDMADescBuffer(udmaBuff, udmaDummyStartDescOffset, allNspsMask);

  // VTCM Usage
  metadata->setVTCMSize(vtcmBuffersSize);

  // L2TCM Usage
  metadata->setL2TCMSize(l2tcmBuffersSize);

  // Shared DDR Usage
  metadata->setStaticSharedDDRSize(ddrBuffersSize);
  metadata->setStaticSharedDDRECCEnabled(false);
  metadata->setDynamicSharedDDRSize(0);
  metadata->setDynamicSharedDDRECCEnabled(false);

  // Network Heap
  metadata->setNetworkHeapSize(nm_proto.heapsize());

  // Single VTCM Page
  metadata->setSingleVTCMPage(nm_proto.singlevtcmpage());

  // Program Description Constants file
  std::ofstream constantsFile;
  constantsFile.open("constants.bin", std::ios::binary | std::ios::out);
  if (!constantsFile) {
    llvm::errs() << "Unable to create constants.bin\n";
    exit(-1);
  }
  uint32_t progDescSize = progDesc.serialize(constantsFile);
  constantsFile.close();

  // Constants Usage
  metadata->setStaticConstantsSize(progDescSize);
  metadata->setStaticConstantsECCEnabled(false);
  metadata->setDynamicConstantsSize(0);
  metadata->setDynamicConstantsECCEnabled(false);
  metadata->addConstantMapping(allNspsMask, /*offset*/ 0, progDescSize);

  // Init constant descriptor bin.
  std::ofstream constDescFile;
  constDescFile.open("constantsdesc.bin", std::ios::binary | std::ios::out);
  if (!constDescFile) {
    llvm::errs() << "Unable to create constantsdesc.bin\n";
    exit(-1);
  }
  AICConstantDescriptor constDesc = {
      .staticConstantsSize = metadata->getStaticConstantsSize(),
      .dynamicConstantsSize = metadata->getDynamicConstantsSize(),
      .staticConstantsECCEnabled = metadata->getStaticConstantsECCEnabled(),
      .dynamicConstantsECCEnabled = metadata->getDynamicConstantsECCEnabled(),
  };
  constDescFile.write((char *)&constDesc, sizeof(constDesc));
  constDescFile.close();

  // Setup HVX threads
  nnc_activate_fp entryPoint =
      reinterpret_cast<nnc_activate_fp>(getEntrypointAddr());
  for (unsigned int i = 0; i < numHvxThreads; i++)
    metadata->addThreadDescriptor(entryPoint, false, true);

  // Setup HMX thread
  for (unsigned int i = 0; i < numHmxThreads; i++)
    metadata->addThreadDescriptor(entryPoint, true, false);

  metadata->finalize();

  return metadata;
}

std::unique_ptr<aicnwdesc::networkDescriptor>
ComputeProgram::generateNetworkDescriptor() const {
  auto &nm_proto = config_.get();
  std::unique_ptr<aicnwdesc::networkDescriptor> nw_proto{
      new aicnwdesc::networkDescriptor()};

  int bufNum = 0;
  int i = 0;

  nw_proto->set_major_version(AIC_NETWORK_DESCRIPTION_MAJOR_VERSION);
  nw_proto->set_minor_version(AIC_NETWORK_DESCRIPTION_MINOR_VERSION);
  nw_proto->set_network_name(nm_proto.name());
  nw_proto->set_num_cores(nm_proto.numnsps());

  uint32_t numThreads, numHmxThreads, numHvxThreads;
  getNumThreads(numThreads, numHmxThreads, numHvxThreads, nm_proto);
  nw_proto->set_num_threads(numThreads);
  nw_proto->set_num_hvx_threads(numHvxThreads);
  // Add thread_groups
  for (i = 0; i < numThreads; i++) {
    nw_proto->add_thread_groups(i < numHvxThreads ? 0 : 1);
  }
  const std::vector<std::string> threadGroups = {"HVX", "HMX"};
  nw_proto->set_thread_group_issue_count(threadGroups.size());
  // Add thread_group_names
  for (i = 0; i < threadGroups.size(); i++) {
    nw_proto->add_thread_group_names(threadGroups[i]);
  }
  nw_proto->set_batch_size(1);
  // don't add RuntimeLoadableConstant runtime_loadable_constants (name, size,
  // offset)
  // Add cluster_offsets - just one at offset 0
  nw_proto->add_cluster_offsets(0);

  std::string buffName;
  /* Process Inputs */
  for (i = 0; i < nm_proto.inputs_size(); i++, bufNum++) {
    auto *in = nw_proto->add_inputs();
    buffName = llvm::formatv("inputBuff_{0}", i);
    in->set_name(buffName.c_str());
    in->set_is_partial_allowed(nm_proto.inputs(i).allowpartial());
    in->set_align(getTypeSize(
        nm_proto.inputs(i)
            .type()));
    assert((nm_proto.inputs(i).dims_size() == 1) ||
           !nm_proto.inputs(i).allowpartial() &&
               "Partial buffers must have only 1 dim");
    in->mutable_io_initial()->set_type(nm_proto.inputs(i).type());
    for (int dim = 0; dim < nm_proto.inputs(i).dims_size(); dim++) {
      in->mutable_io_initial()->add_dims(nm_proto.inputs(i).dims(dim));
    }
    in->mutable_io_initial()->set_layout(
        ::aicnwdesc::networkDescLayout::FlatNXYD);

    in->mutable_io_transformed()->set_type(nm_proto.inputs(i).type());
    for (int dim = 0; dim < nm_proto.inputs(i).dims_size(); dim++) {
      in->mutable_io_transformed()->add_dims(nm_proto.inputs(i).dims(dim));
    }
    in->mutable_io_transformed()->set_layout(
        ::aicnwdesc::networkDescLayout::FlatNXYD);

    auto *copyDMACtrl = in->add_transformseq();
    copyDMACtrl->set_kind(aicnwdesc::CopyDMABufferTransform);
    copyDMACtrl->set_type(nm_proto.inputs(i).type());
    copyDMACtrl->set_scale(1);
    copyDMACtrl->set_offset(0);
    for (int dim = 0; dim < nm_proto.inputs(i).dims_size(); dim++) {
      copyDMACtrl->add_dims(nm_proto.inputs(i).dims(dim));
    }
    copyDMACtrl->set_layout(::aicnwdesc::networkDescLayout::FlatNXYD);
    copyDMACtrl->mutable_copy_dma_buffer()->set_dir(aicnwdesc::direction::In);
    copyDMACtrl->mutable_copy_dma_buffer()->set_offset(0);
    copyDMACtrl->mutable_copy_dma_buffer()->set_buffer_num(bufNum);

    auto *dmaBuffer = nw_proto->add_dma_buffers();
    dmaBuffer->set_size(getIOSize(nm_proto.inputs(i)));
    dmaBuffer->set_dir(aicnwdesc::In);
  }

  /* Process Outputs */
  for (i = 0; i < nm_proto.outputs_size(); i++, bufNum++) {
    auto *out = nw_proto->add_outputs();
    buffName = llvm::formatv("outputBuff_{0}", i);
    out->set_name(buffName.c_str());
    out->mutable_io_initial()->set_type(nm_proto.outputs(i).type());
    for (int dim = 0; dim < nm_proto.outputs(i).dims_size(); dim++) {
      out->mutable_io_initial()->add_dims(nm_proto.outputs(i).dims(dim));
    }
    out->mutable_io_initial()->set_layout(
        ::aicnwdesc::networkDescLayout::FlatNXYD);

    out->mutable_io_transformed()->set_type(nm_proto.outputs(i).type());
    for (int dim = 0; dim < nm_proto.outputs(i).dims_size(); dim++) {
      out->mutable_io_transformed()->add_dims(nm_proto.outputs(i).dims(dim));
    }
    out->mutable_io_transformed()->set_layout(
        ::aicnwdesc::networkDescLayout::FlatNXYD);

    auto *copyDMACtrl = out->add_transformseq();
    copyDMACtrl->set_kind(aicnwdesc::CopyDMABufferTransform);
    copyDMACtrl->set_type(nm_proto.outputs(i).type());
    copyDMACtrl->set_scale(1);
    copyDMACtrl->set_offset(0);
    for (int dim = 0; dim < nm_proto.outputs(i).dims_size(); dim++) {
      copyDMACtrl->add_dims(nm_proto.outputs(i).dims(dim));
    }
    copyDMACtrl->set_layout(::aicnwdesc::networkDescLayout::FlatNXYD);
    copyDMACtrl->mutable_copy_dma_buffer()->set_dir(aicnwdesc::direction::Out);
    copyDMACtrl->mutable_copy_dma_buffer()->set_offset(0);
    copyDMACtrl->mutable_copy_dma_buffer()->set_buffer_num(bufNum);

    auto *dmaBuffer = nw_proto->add_dma_buffers();
    dmaBuffer->set_size(getIOSize(nm_proto.outputs(i)));
    dmaBuffer->set_dir(aicnwdesc::Out);
  }

  return nw_proto;
}

bool ComputeProgram::validateQPC(QAicQpcHandle *handle) const {
  // Make sure the QPC has the epxtected segments
  QAicQpc *qpc = nullptr;
  if (getQpc(handle, &qpc) != 0) {
    llvm::errs() << "Failed to get QPC from handle.\n";
    return false;
  }

  if (qpc->hdr.magicNumber != AICQPC_MAGIC_NUMBER) {
    llvm::errs() << "QPC magic number mismatch: " << qpc->hdr.magicNumber
                 << "\n";
    return false;
  }

  // Too few QPC segments
  if (qpc->numImages < 2) {
    return false;
  }

  std::set<StringRef> requiredSections;
  requiredSections.insert("network.elf");
  requiredSections.insert("networkdesc.bin");

  // Check if we need to also require constant sections
  // If we enounter any related section then all are requires
  for (uint64_t i = 0; i < qpc->numImages; ++i) {
    StringRef nameRef{qpc->images[i].name};
    if (nameRef == "constants.bin" || nameRef == "constantsdesc.bin") {
      requiredSections.insert("constants.bin");
      requiredSections.insert("constantsdesc.bin");
      break;
    }
  }

  // Pop off the required sections as we find them any remaining are MIA
  for (uint64_t i = 0; i < qpc->numImages; ++i) {
    StringRef nameRef{qpc->images[i].name};
    if (requiredSections.count(nameRef)) {
      requiredSections.erase(nameRef);
    }
  }

  if (requiredSections.size() > 0) {
    for (auto &s : requiredSections) {
      llvm::dbgs() << "QPC missing required section: " << s.str() << "\n";
    }
    return false;
  }

  return true;
}
