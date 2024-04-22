// Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef FLATBUFF_DRIVER_FLATBUF_DECODE_H
#define FLATBUFF_DRIVER_FLATBUF_DECODE_H
#include "AICMetadata.h"
#include "AICMetadataReader.h"
#include "AICMetadataWriter.h"
#include "AicMetadataFlat_generated.h"
#include "metadataFlatbufferWriter.h"
#include "metadata_flat_knownFields.h"
#include <cstring>
#include <string>

namespace metadata {

static const std::string networkElfMetadataSection("metadata");
static const std::string networkElfMetadataFBSection_legacy("metadata_fb");

class FlatDecode {
public:
  //------------------------------------------------------------------------
  // The "terminator metadata" is a 4 byte header that goes
  // in front of metadata, such that section["metadata"]=
  //[term metadata][metadata in flatbuffer format].
  // Input: flatbuf that may have a terminator metadata header
  // Output: flatbuf with terminator metadata header removed
  //------------------------------------------------------------------------
  static auto stripTerminatorMetadata(std::vector<uint8_t> &flatbuf) {
    (void)networkElfMetadataSection;
    (void)networkElfMetadataFBSection_legacy;
    if (!flatbufVerifyMetadataBuffer(flatbuf) &&
        flatbuf.size() > sizeof(termMetadata)) {
      miniMetadata header;
      memcpy(&header, flatbuf.data(), sizeof(termMetadata));
      if (header.versionMajor == termMetadata.versionMajor &&
          header.versionMinor == termMetadata.versionMinor) {
        // it failed to verify because the header is a termMetadata
        flatbuf.erase(flatbuf.begin(), flatbuf.begin() + sizeof(termMetadata));
      }
    }
    return flatbuf;
  }
  //------------------------------------------------------------------------
  // Same as readMetadataFlat but returns a C++ STL object of the flatbuffer
  // if result string is empty, this function worked
  //------------------------------------------------------------------------
  static auto readMetadataFlatNativeCPP(std::vector<uint8_t> &flatbuf,
                                        std::string &resultString) {
    std::unique_ptr<AicMetadataFlat::MetadataT> retvalue;
    resultString = ""; // empty result string means it worked
    flatbuf = stripTerminatorMetadata(flatbuf);
    if (!flatbufVerifyMetadataBuffer(flatbuf)) {
      resultString += " metadataflat failed to verify \n";
      return retvalue;
    }
    if (!flatbufVerifyCompatibility(flatbuf)) {
      resultString += " metadataflat is incompatible \n";
      return retvalue;
    }
    return AicMetadataFlat::UnPackMetadata((const uint8_t *)&flatbuf[0]);
  }
  //------------------------------------------------------------------------
  // Precondition: some buffer of size bytes that may be a flatbuffer, an empty
  //      string to print any results to.
  // Postcondition: (1) buffer validated as instance of AicMetadataFlat
  //                (2) major versions match
  //                (3) all requiredFields inside AicMetadataFlat are in the
  //                 array known_AicMetadataFlat_fields
  //                (4) flatbuffer version returned as constant readable object
  //                 using auto generated accessor functions
  //                (5) if result string is empty, this function worked
  //------------------------------------------------------------------------
  static const AicMetadataFlat::Metadata *
  readMetadataFlat(std::vector<uint8_t> &flatbuf, std::string &resultString) {
    resultString = ""; // empty result string means it worked
    flatbuf = stripTerminatorMetadata(flatbuf);
    if (!flatbufVerifyMetadataBuffer(flatbuf)) {
      resultString += " metadataflat failed to verify \n";
      return nullptr;
    }
    if (!flatbufVerifyCompatibility(flatbuf)) {
      resultString += " metadataflat is incompatible \n";
      return nullptr;
    }
    return AicMetadataFlat::GetMetadata(&flatbuf[0]);
  }
  //------------------------------------------------------------------------
  // Precondition: some buffer of size bytes that may be a flatbuffer
  // Postcondition: (1) buffer validated as instance of AicMetadataFlat
  //                (2) major versions match
  //                (3) all requiredFields inside AicMetadataFlat are in the
  //                array
  // known_AicMetadataFlat_fields (4) flatbuffer version translated to struct
  // version of metadata Returnvalue: an instance of aic_metadata
  //------------------------------------------------------------------------
  template <class W>
  static auto
  flatbufValidateTranslateAicMetadata(std::vector<uint8_t> &flatbuf) {
    std::string errString;
    auto metadataAsFlat = readMetadataFlat(flatbuf, errString);
    if (metadataAsFlat == nullptr) {
      std::cout << errString << std::flush;
      throw;
    }
    return translateToMetadata<W>(metadataAsFlat);
  }

  // purpose: same as other version, but returns an STL vector instead
  template <class W>
  static const std::vector<uint8_t>
  flatbufValidateTranslateAicMetadataVector(std::vector<uint8_t> &flatbuf) {
    auto tempAicmetadata = flatbufValidateTranslateAicMetadata<W>(flatbuf);
    auto temp = tempAicmetadata.getMetadata();
    return temp;
  }
  static bool flatbufValidate(const std::vector<uint8_t> &flatbuf) {
    if (flatbufVerifyMetadataBuffer(flatbuf) &&
        flatbufVerifyCompatibility(flatbuf)) {
      return true;
    }
    return false;
  }
  static bool flatbufVerifyMetadataBuffer(const std::vector<uint8_t> &flatbuf) {
    flatbuffers::Verifier verify((const uint8_t *)&flatbuf[0], flatbuf.size());
    bool ok = AicMetadataFlat::VerifyMetadataBuffer(verify);
    if (!ok) {
      return false;
    }
    return true;
  }

private:
  static bool flatbufVerifyCompatibility(const std::vector<uint8_t> &flatbuf) {
    auto metadataAsFlat =
        AicMetadataFlat::GetMetadata((const uint8_t *)&flatbuf[0]);
    if (!compatibilityCheck(metadataAsFlat)) {
      return false;
    }
    return true;
  }

  template <class W>
  static W
  translateToMetadata(const AicMetadataFlat::Metadata *metadataAsFlat) {
    W aicMetadata =
        W(metadataAsFlat->hwVersionMajor(), metadataAsFlat->hwVersionMinor(),
          metadataAsFlat->versionMajor(), metadataAsFlat->versionMinor(),
          metadataAsFlat->execContextMajorVersion(),
          metadataAsFlat->exitDoorbellOffset());

    translateMetaDataScalars(&aicMetadata, metadataAsFlat);
    translateMetadataMultiCastEntries(&aicMetadata, metadataAsFlat);
    translateMetadataHostMultiCastEntries(&aicMetadata, metadataAsFlat);
    translateMetadataL2TCMInitState(&aicMetadata, metadataAsFlat);
    translateMetadataSemaphoreInitState(&aicMetadata, metadataAsFlat);
    translateMetadataThreadDescriptors(&aicMetadata, metadataAsFlat);
    translateMetadataConstantMappings(&aicMetadata, metadataAsFlat);
    translateMetadataDMARequests(&aicMetadata, metadataAsFlat);
    translateMetadataPortTable(&aicMetadata, metadataAsFlat);

    aicMetadata.finalize();
    return aicMetadata;
  }

  // return true if all requiredFields[] in known_AicMetadataFlat_fields
  static bool
  introspectionCheck(const AicMetadataFlat::Metadata *metadataAsFlat) {
    if (metadataAsFlat->requiredFields() == nullptr)
      return true;
    std::vector<std::string> missingFields;
    auto requiredFieldsVector = metadataAsFlat->requiredFields();
    for (size_t i = 0; i < requiredFieldsVector->size(); i++) {
      auto required_field = requiredFieldsVector->Get(i)->str();
      if (!searchKnownFields(required_field)) {
        missingFields.push_back(required_field);
      }
    }
    if (!missingFields.empty()) {
      std::cout << "Network features unsupported by this Platform version "
                   "encountered:  "
                << std::endl;
      for (std::string s : missingFields) {
        std::cout << s << std::endl;
      }
      std::cout << ".  A newer version of the platform is required to run this "
                   "network."
                << std::endl;
      return false;
    }
    return true;
  }

  // return true if major version matches and all requiredFields[] in
  // known_AicMetadataFlat_fields
  static bool
  compatibilityCheck(const AicMetadataFlat::Metadata *metadataAsFlat) {
    if (metadataAsFlat->versionMajor() != AIC_METADATA_MAJOR_VERSION) {
      std::cout << "Metadata major version decoded = "
                << metadataAsFlat->versionMajor() << ", not matching known "
                << AIC_METADATA_MAJOR_VERSION << std::endl;
      return false;
    }
    if (!introspectionCheck(metadataAsFlat)) {
      return false;
    }
    return true;
  }

  // -----------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains simple fields not in *aicMetadata
  // Postcondition: aicMetadata contains the same fields
  // -----------------------------------------------------------------------------
  template <class W>
  static void
  translateMetaDataScalars(W *aicMetadata,
                           const AicMetadataFlat::Metadata *metadataAsFlat) {

    aicMetadata->setNetworkName(metadataAsFlat->networkName()->c_str());

    aicMetadata->setStaticSharedDDRSize(metadataAsFlat->staticSharedDDRSize());
    aicMetadata->setDynamicSharedDDRSize(
        metadataAsFlat->dynamicSharedDDRSize());
    aicMetadata->setStaticConstantsSize(metadataAsFlat->staticConstantsSize());
    aicMetadata->setDynamicConstantsSize(
        metadataAsFlat->dynamicConstantsSize());
    aicMetadata->setExitDoorbellOffset(metadataAsFlat->exitDoorbellOffset());
    aicMetadata->setNumSemaphores(metadataAsFlat->numSemaphores());
    aicMetadata->setStaticSharedDDRECCEnabled(
        metadataAsFlat->staticSharedDDRECCEnabled());
    aicMetadata->setDynamicSharedDDRECCEnabled(
        metadataAsFlat->dynamicSharedDDRECCEnabled());
    aicMetadata->setStaticConstantsECCEnabled(
        metadataAsFlat->staticConstantsECCEnabled());
    aicMetadata->setDynamicConstantsECCEnabled(
        metadataAsFlat->dynamicConstantsECCEnabled());
    aicMetadata->setSingleVTCMPage(metadataAsFlat->singleVTCMPage());
    aicMetadata->setVTCMSize(metadataAsFlat->VTCMSize());
    aicMetadata->setL2TCMSize(metadataAsFlat->L2TCMSize());
    aicMetadata->setNetworkHeapSize(metadataAsFlat->networkHeapSize());
    aicMetadata->setNumNSPs(metadataAsFlat->numNSPs());
    aicMetadata->setVTCMSize(metadataAsFlat->VTCMSize());
    aicMetadata->setL2TCMSize(metadataAsFlat->L2TCMSize());
    aicMetadata->setExitDoorbellOffset(metadataAsFlat->exitDoorbellOffset());
    aicMetadata->initL2TCMResize(metadataAsFlat->L2TCMInitSize());
    aicMetadata->setHasHmxFP(metadataAsFlat->hasHmxFP());
    aicMetadata->setHasHvxFP(metadataAsFlat->hasHvxFP());
    aicMetadata->set_raw_struct_version_length(
        metadataAsFlat->raw_struct_version_length());
    auto requiredFieldsVector = metadataAsFlat->requiredFields();
    for (size_t i = 0; i < requiredFieldsVector->size(); i++) {
      aicMetadata->addRequiredField(requiredFieldsVector->Get(i)->str());
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains semaphore fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataSemaphoreInitState(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto source_vector = metadataAsFlat->semaphoreInitState();
    for (size_t i = 0; i < source_vector->size(); i++) {
      aicMetadata->initSemaphore(i, source_vector->Get(i));
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains DMARequest fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataDMARequests(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto sourceVector = metadataAsFlat->dmaRequests();
    if (sourceVector != NULL) {
      for (size_t i = 0; i < sourceVector->size(); i++) {
        SemaphoreOps semopsvec;
        auto theseSemaOps = (sourceVector->Get(i))->semaphoreOps();
        if (theseSemaOps != NULL) {
          for (size_t j = 0; j < theseSemaOps->size(); j++) {
            auto thisop = theseSemaOps->Get(j);
            aicMetadata->addSemaphoreOp(
                semopsvec, thisop->semOp(), thisop->semNum(),
                thisop->semValue(), thisop->preOrPost(), thisop->inSyncFence(),
                thisop->outSyncFence());
          }
        }
        DoorbellOps dbopsvec;
        auto deseDorbOps = (sourceVector->Get(i))->doorbellOps();
        if (deseDorbOps != NULL) {
          for (size_t j = 0; j < deseDorbOps->size(); j++) {
            auto thisop = deseDorbOps->Get(j);
            aicMetadata->addDoorbellOp(dbopsvec, thisop->size(), thisop->mcId(),
                                       thisop->offset(), thisop->data());
          }
        }
        auto thisrequest = sourceVector->Get(i);
        aicMetadata->addDMARequest(
            thisrequest->num(), thisrequest->hostOffset(),
            thisrequest->devAddrSpace(), thisrequest->devOffset(),
            thisrequest->size(), thisrequest->inOut(), thisrequest->portId(),
            thisrequest->mcId(), semopsvec, dbopsvec,
            thisrequest->transactionId());
      }
    }
  }
  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains MultiCast Entry fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataMultiCastEntries(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto sourceVector = metadataAsFlat->nspMulticastTables();
    for (size_t core = 0; core < sourceVector->size(); core++) {
      auto entries = sourceVector->Get(core);
      auto numMulticastEntries = entries->multicastEntries()->size();
      for (size_t j = 0; j < numMulticastEntries; j++) {
        auto anentry = entries->multicastEntries()->Get(j);
        aicMetadata->addNSPMulticastEntry(
            core, anentry->dynamic(), anentry->mask(), anentry->size(),
            anentry->addrSpace(), anentry->baseAddrOffset());
      }
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains MultiCast Entry fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataHostMultiCastEntries(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto sourceVector =
        metadataAsFlat->hostMulticastTable()->multicastEntries();
    for (size_t i = 0; i < sourceVector->size(); i++) {
      aicMetadata->addHostMulticastEntry(sourceVector->Get(i)->mask(),
                                         sourceVector->Get(i)->size());
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains L2TCInit fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataL2TCMInitState(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto sourceVector = metadataAsFlat->L2TCMInitState();
    for (size_t i = 0; i < sourceVector->size(); i++) {
      aicMetadata->initL2TCMByte(i, sourceVector->Get(i));
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains ThreadDescriptor fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataThreadDescriptors(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto sourceVector = metadataAsFlat->threadDescriptors();
    if (sourceVector != NULL) {
      for (size_t i = 0; i < sourceVector->size(); i++) {
        auto aDescriptor = sourceVector->Get(i);
        aicMetadata->addThreadDescriptor(
            (nnc_activate_fp)aDescriptor->entryPoint(),
            aDescriptor->typeMask());
      }
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains ConstantMapping fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void translateMetadataConstantMappings(
      W *aicMetadata, const AicMetadataFlat::Metadata *metadataAsFlat) {
    auto sourceVector = metadataAsFlat->constantMappings();
    if (sourceVector != NULL) {
      for (size_t i = 0; i < sourceVector->size(); i++) {
        auto aMapping = sourceVector->Get(i);
        aicMetadata->addConstantMapping(aMapping->coreMask(),
                                        aMapping->constantDataBaseOffset(),
                                        aMapping->size());
      }
    }
  }

  //------------------------------------------------------------------------
  // Precondition:  metadataAsFlat contains portTable fields not in
  // *aicMetadata Postcondition: aicMetadata contains the same fields
  //------------------------------------------------------------------------
  template <class W>
  static void
  translateMetadataPortTable(W *aicMetadata,
                             const AicMetadataFlat::Metadata *metadataAsFlat) {
    const auto &sourcePortTable = metadataAsFlat->portTable();
    if (sourcePortTable != nullptr) {
      for (const auto &port : *sourcePortTable) {
        aicMetadata->addPort(port->portId(), port->portType());
      }
    }
  }

  // return true if requiredField in known_AicMetadataFlat_fields
  static bool searchKnownFields(std::string requiredField) {
    // hashmap would be O(n)
    // bin search would be O(n log n)
    // doing O(n^2) for now
    (void)known_AicMetadataFlat_fields_names;
    for (unsigned int i = 0; i < known_fields_length; i++) {
      if ((requiredField.size() == known_AicMetadataFlat_fields_length[i]) &&
          (strncmp(requiredField.c_str(), known_AicMetadataFlat_fields[i],
                   known_AicMetadataFlat_fields_length[i]) == 0))
        return true;
    }
    return false;
  }
};
//------------------------------------------------------------------------
// This prints the flatbuffer in the same "dump" format that you can view
// in AICMetadata.  Implemented by conversion as the conversion is unit
// tested.
//------------------------------------------------------------------------
inline void dump(std::vector<uint8_t> &flatbuf_bytes, FILE *fd = nullptr) {
  auto vec = metadata::FlatDecode::flatbufValidateTranslateAicMetadataVector<
      AICMetadataWriter>(flatbuf_bytes);
  constexpr int errBufLen = 1024;
  char errbuf[errBufLen] = {0};
  auto metadata = MDR_readMetadata(vec.data(), vec.size(), errbuf, errBufLen);
  if (!metadata) {
    throw std::runtime_error("Failed to parse metadata " + std::string(errbuf));
  }
  AICMetadata_dump(metadata, fd);
}
//------------------------------------------------------------------------
// Input: metadata for a network in C++ data structure
// Output : bytes required of DRAM on the soc to execute the network
//------------------------------------------------------------------------
[[nodiscard]] inline uint64_t
metadataGetTotalRequiredMemory(const AicMetadataFlat::MetadataT *metadata) {
  constexpr auto SOC_PADDING =
      2048; // these memory buffers have to be aligned to 2048 bytes
  constexpr auto PADDING_MULTIPLIER = 4; // just to be on the safe side
  return metadata->staticSharedDDRSize + metadata->staticConstantsSize +
         metadata->dynamicSharedDDRSize + metadata->dynamicConstantsSize +
         SOC_PADDING * PADDING_MULTIPLIER;
}

} // namespace metadata
#endif // FLATBUFF_DRIVER_FLATBUF_DECODE_H
