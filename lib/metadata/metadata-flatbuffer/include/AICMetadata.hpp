// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef AIC_METADATA_HPP_
#define AIC_METADATA_HPP_
#include "AicMetadataFlat_generated.h"
#include "metadataflatbufDecode.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
[[nodiscard]] auto inline dumpSemaphoreInitState(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  for (auto i = 0; i < metadata->numSemaphores; ++i) {
    buffer << "    " << std::dec << std::setw(3) << i << ": 0x" << std::hex
           << metadata->semaphoreInitState[i] << "\n";
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpL2TCMInitState(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  constexpr auto dispSize = 4;
  for (uint32_t i = 0; i < metadata->L2TCMInitSize / dispSize; ++i) {
    if (i && (i % 8 == 0)) {
      buffer << "\n";
    }
    using destType = uint32_t;
    // read 32 bit wide version of the data by bit shifting, little endian
    destType wide_version = static_cast<destType>(metadata->L2TCMInitState[i * dispSize]) |
                            static_cast<destType>(metadata->L2TCMInitState[i * dispSize + 1]) << 8 |
                            static_cast<destType>(metadata->L2TCMInitState[i * dispSize + 2]) << 16 |
                            static_cast<destType>(metadata->L2TCMInitState[i * dispSize + 3]) << 24;
    buffer << " " << std::hex << std::setw(8) << std::setfill('0')
           << wide_version;
  }
  buffer << "\n";
  return buffer.str();
}
[[nodiscard]] auto inline dumpSemaphoreOp(
    const AicMetadataFlat::AICMDSemaphoreOpT &o, int idx) {
  std::ostringstream buffer;
  std::string opStr;
  switch (o.semOp) {
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdNOP:
    opStr = "nop";
    break;
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdINIT:
    opStr = "init";
    break;
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdINC:
    opStr = "inc";
    break;
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdDEC:
    opStr = "dec";
    break;
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdWAITEQ:
    opStr = "waiteq";
    break;
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdWAITGE:
    opStr = "waitge";
    break;
  case AicMetadataFlat::AICMDSemaphoreOpcode_AICMDSemaphoreCmdP:
    opStr = "waitgezerodec";
    break;
  default:
    assert(0 && "Unknown semaphore operation.");
  }
  buffer << "  semaphoreOp[" << idx << "] - semOp: " << std::setw(6) << opStr
         << " semNum: " << std::dec << std::setw(3) << o.semNum
         << " semValue: " << std::dec << std::setw(3) << o.semValue;
  std::string preOrPostStr;
  switch (o.preOrPost) {
  case AicMetadataFlat::AICMDSemaphoreSync_AICMDSemaphoreSyncPost:
    preOrPostStr = "post";
    break;
  case AicMetadataFlat::AICMDSemaphoreSync_AICMDSemaphoreSyncPre:
    preOrPostStr = "pre";
    break;
  default:
    assert(0 && "Unknown semaphore pre/post.");
  }
  buffer << " preOrPost: " << std::setw(4) << preOrPostStr;
  buffer << " inSyncFence: " << std::dec << std::setw(3) << +o.inSyncFence
         << " outSyncFence: " << std::dec << std::setw(3) << +o.outSyncFence
         << "\n";
  return buffer.str();
}
[[nodiscard]] auto inline dumpDoorbellOp(
    const AicMetadataFlat::AICMDDoorbellOpT &d) {
  std::ostringstream buffer;
  auto idx = 0;
  buffer << "  doorbellOp[" << idx++ << "]  - size: " << std::setw(3) << +d.size
         << " mcId: " << std::setw(3) << d.mcId << " offset: " << std::setw(8)
         << d.offset << " data: " << std::setw(3) << d.data << "\n";
  return buffer.str();
}
[[nodiscard]] auto inline dumpDMARequest(
    const AicMetadataFlat::AICMDDMARequestT &r) {
  std::ostringstream buffer;
  static auto idx = 0;
  buffer << " DMA Request[" << idx++ << "] -";
  buffer << " num: " << r.num;
  buffer << " hostOffset: " << r.hostOffset;
  std::string devAddrSpaceStr;
  switch (r.devAddrSpace) {
  case AicMetadataFlat::AICMDDMAEntryAddrSpace_AICMDDMAAddrSpaceMC:
    devAddrSpaceStr = "MC";
    break;
  case AicMetadataFlat::AICMDDMAEntryAddrSpace_AICMDDMAAddrSpaceDDR:
    devAddrSpaceStr = "DDR";
    break;
  case AicMetadataFlat::
      AICMDDMAEntryAddrSpace_AICMDDMAAddrSpaceDDRDynamicShared:
    devAddrSpaceStr = "DDRDynamicShared";
    break;
  default:
    assert(0 && "Unknown DMA address space.");
  }
  buffer << " devAddrSpace: " << devAddrSpaceStr;
  buffer << " devOffset: " << r.devOffset;
  buffer << " size: " << r.size << " mcId: " << r.mcId;
  std::string inOutStr;
  switch (r.inOut) {
  case AicMetadataFlat::AICMDDMADirection_AICMDDMAIn:
    inOutStr = "in";
    break;
  case AicMetadataFlat::AICMDDMADirection_AICMDDMAOut:
    inOutStr = "out";
    break;
  default:
    assert(0 && "Unknown DMA in/out param.");
  }
  buffer << " inOut: " << inOutStr << " portId: " << r.portId;
  buffer << " transactionId: " << r.transactionId << "\n";
  int semIdx = 0;
  for (const auto &semaphoreOp : r.semaphoreOps) {
    buffer << dumpSemaphoreOp(*semaphoreOp, semIdx++);
  }
  for (const auto &doorbellOp : r.doorbellOps) {
    buffer << dumpDoorbellOp(*doorbellOp);
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpDMARequests(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  for (const auto &request : metadata->dmaRequests) {
    buffer << "\n";
    buffer << dumpDMARequest(*request);
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpPortTable(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  buffer << "\n" << "PortTable:" << "\n";
  buffer << " numPorts: " << metadata->portTable.size() << "\n";
  int i = 0;
  for (const auto &port : metadata->portTable) {
    std::string portTypeStr;
    switch (port.portType()) {
    case AicMetadataFlat::AICMDPortType_AICMDPortUserIO:
      portTypeStr = "UserIO";
      break;
    case AicMetadataFlat::AICMDPortType_AICMDPortP2P:
      portTypeStr = "P2P";
      break;
    case AicMetadataFlat::AICMDPortType_AICMDPortMDP:
      portTypeStr = "MDP";
      break;
    default:
      assert(0 && "Invalid port type.");
    }
    buffer << "  [" << i++ << "]: Id: " << std::setw(3) << port.portId()
           << " Type: " << portTypeStr << "\n";
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpHostMulticastTable(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  buffer << "  hostMulticastTable:" << "\n";
  auto table = metadata->hostMulticastTable.get();
  assert(table && "Invalid table");
  buffer << "    numMulticastEntries: " << std::dec << std::setw(3)
         << table->multicastEntries.size() << "\n";
  auto MCID = 0;
  // Checking if the exposed size field is present in host MCID table
  bool isExposedFieldPresent = false;
  std::string searchExposedSizeStr = "AicMetadataFlat_Metadata->hostMulticastTable->multicastEntries->exposedSize";
  auto itr = std::find(metadata->requiredFields.begin(), metadata->requiredFields.end(), searchExposedSizeStr);
  if (itr != metadata->requiredFields.end())
    isExposedFieldPresent = true;
  for (const auto &entry : table->multicastEntries) {
    buffer << "    MCID: " << std::setw(2) << MCID++
           << " mask: " << std::setw(4) << entry->mask
           << " size: " << std::setw(8) << entry->size;
      buffer << "\n";
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpThreadDescriptors(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  buffer << "\n" << "Thread Descriptors:" << "\n";
  buffer << " numThreadDescriptors: " << metadata->threadDescriptors.size()
         << "\n";
  auto threadId = 0;
  for (const auto &desc : metadata->threadDescriptors) {
    std::string typeStr = "none";
    if ((desc->typeMask & AicMetadataFlat::AICMDThreadType_AICMDThreadHMX) &&
        (desc->typeMask & AicMetadataFlat::AICMDThreadType_AICMDThreadHVX)) {
      typeStr = "hmx+hvx";
    } else if (desc->typeMask &
               AicMetadataFlat::AICMDThreadType_AICMDThreadHMX) {
      typeStr = "hmx";
    } else if (desc->typeMask &
               AicMetadataFlat::AICMDThreadType_AICMDThreadHVX) {
      typeStr = "hvx";
    }
    buffer << "  ThreadDesc[" << threadId++ << "]: entryPoint 0x" << std::hex
           << desc->entryPoint << " typeMask: " << typeStr << "\n";
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpConstantMappings(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  buffer << "\n" << "Constant Mappings:" << "\n";
  buffer << " numConstantMappings: " << metadata->constantMappings.size()
         << "\n";
  auto mappingId = 0;
  for (const auto &mapping : metadata->constantMappings) {
    buffer << "  ConstantMapping[" << mappingId++ << "]: mask: " << std::setw(8)
           << mapping->coreMask << " constantDataBaseOffset: " << std::setw(8)
           << mapping->constantDataBaseOffset << " size: " << std::setw(3)
           << mapping->size << "\n";
  }
  return buffer.str();
}
[[nodiscard]] auto inline dumpNSPMulticastTable(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata, int core) {
  std::ostringstream buffer;
  assert(core < 16 && "Invalid core."); // FIXME: magic number.
  buffer << " nspMulticastTable[" << core << "]:"
         << "\n";
  auto table = metadata->nspMulticastTables[core].get();
  assert(table && "Invalid table");
  buffer << "    numMulticastEntries: " << std::dec << std::setw(3)
         << table->multicastEntries.size() << "\n";
  auto MCID = 0;
  for (const auto &entry : table->multicastEntries) {
    buffer << "    MCID: " << std::setw(2) << MCID++
           << " mask: " << std::setw(4) << entry->mask
           << " size: " << std::setw(8) << entry->size
           << " dynamic: " << std::setw(3) << +entry->dynamic << " addrSpace: ";
    std::string addrSpaceStr;
    switch (entry->addrSpace) {
    case AicMetadataFlat::AICMDMulticastEntryAddrSpace_AICMDAddrSpaceL2TCM:
      addrSpaceStr = "L2TCM";
      break;
    case AicMetadataFlat::AICMDMulticastEntryAddrSpace_AICMDAddrSpaceVTCM:
      addrSpaceStr = " VTCM";
      break;
    default:
      assert(0 && "Unknown DMA address space.");
    }
    buffer << addrSpaceStr << " baseAddrOffset: " << std::setw(10)
           << entry->baseAddrOffset << "\n";
  }
  return buffer.str();
}
[[nodiscard]] auto inline dump(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata) {
  std::ostringstream buffer;
  buffer << "AIC Hardware Version " << +metadata->hwVersionMajor << "."
         << +metadata->hwVersionMinor << "\n";
  buffer << "AIC Metadata Version " << metadata->versionMajor << "."
         << metadata->versionMinor << "\n";
  buffer << "AIC ExecContext Major Version "
         << metadata->execContextMajorVersion << "\n";
  buffer << " numNSPs: " << metadata->numNSPs << "\n";
  buffer << " VTCMSize: " << metadata->VTCMSize << "\n";
  buffer << " L2TCMSize: " << metadata->L2TCMSize << "\n";
  buffer << " singleVTCMPage: " << +metadata->singleVTCMPage << "\n";
  buffer << " networkName: " << metadata->networkName << "\n";
  buffer << " numSemaphores: " << metadata->numSemaphores << "\n";
  buffer << "  semaphore initial state:" << "\n";
  buffer << dumpSemaphoreInitState(metadata);
  buffer << " L2TCMInitSize: " << metadata->L2TCMInitSize << "\n";
  buffer << "  L2TCM initial state:" << "\n";
  buffer << dumpL2TCMInitState(metadata);
  buffer << "\n"
         << " StaticSharedDDRSize: " << metadata->staticSharedDDRSize
         << "\n";
  buffer << " StaticSharedDDRECCEnabled: "
         << +metadata->staticSharedDDRECCEnabled << "\n";
  buffer << " DynamicSharedDDRSize: " << metadata->dynamicSharedDDRSize
         << "\n";
  buffer << " DynamicSharedDDRECCEnabled: "
         << +metadata->dynamicSharedDDRECCEnabled << "\n";
  buffer << " Total shared DDR size: "
         << metadata->staticSharedDDRSize + metadata->dynamicSharedDDRSize
         << "\n";
  // Checking if the secure ddr fields are present and set
  auto it1 = std::find(metadata->requiredFields.begin(), metadata->requiredFields.end(), "AicMetadataFlat_Metadata->secureStaticSharedDDRSize");
  auto it2 = std::find(metadata->requiredFields.begin(), metadata->requiredFields.end(), "AicMetadataFlat_Metadata->secureDynamicSharedDDRSize");
  buffer << "\n"
         << " StaticConstantsSize: " << metadata->staticConstantsSize
         << "\n";
  buffer << " StaticConstantsECCEnabled: "
         << +metadata->staticConstantsECCEnabled << "\n";
  buffer << " DynamicConstantsSize: " << metadata->dynamicConstantsSize
         << "\n";
  buffer << " DynamicConstantsECCEnabled: "
         << +metadata->dynamicConstantsECCEnabled << "\n";
  buffer << " Total constants size: "
         << metadata->staticConstantsSize + metadata->dynamicConstantsSize
         << "\n";
  buffer << "\n"
         << " NetworkHeapSize: " << metadata->networkHeapSize << "\n";
  buffer << "\n" << "DMARequests:" << "\n";
  buffer << " numDMARequests: " << metadata->dmaRequests.size() << "\n";
  buffer << dumpDMARequests(metadata);
  buffer << dumpPortTable(metadata);
  buffer << "\n" << "Multicast Tables:" << "\n";
  for (auto core = 0; core < metadata->numNSPs; ++core) {
    buffer << dumpNSPMulticastTable(metadata, core);
  }
  buffer << dumpHostMulticastTable(metadata);
  buffer << dumpThreadDescriptors(metadata);
  buffer << dumpConstantMappings(metadata);
  buffer << "\n"
         << " exitDoorbellOffset: " << metadata->exitDoorbellOffset
         << "\n";
  buffer << "\n"
         << "hasHvxFP: " << (metadata->hasHvxFP ? "true" : "false")
         << "\n";
  buffer << "hasHmxFP: " << (metadata->hasHmxFP ? "true" : "false")
         << "\n";
  return buffer;
}
void inline saveText(const std::string &save_path, const std::string &text) {
  std::ofstream outfile(save_path, std::ostringstream::out);
  if (!outfile.is_open())
    throw std::runtime_error("Unable to open file for writing: " + save_path);
  outfile << text;
}
std::string inline AICMetadata_dump(
    const std::unique_ptr<AicMetadataFlat::MetadataT> &metadata,
    const std::string &save_path = "") {
  auto dumped_text = dump(metadata).str();
  if (!save_path.empty()) {
     saveText(save_path, dumped_text);
  }
  return dumped_text;
}
#endif // AIC_METADATA_HPP_
