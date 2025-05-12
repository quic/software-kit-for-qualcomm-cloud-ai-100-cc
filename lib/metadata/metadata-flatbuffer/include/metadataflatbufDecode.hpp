// Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef FLATBUFF_DRIVER_FLATBUF_DECODE_H
#define FLATBUFF_DRIVER_FLATBUF_DECODE_H
#include "AICMetadata.hpp"
#include "AicMetadataFlat_generated.h"
#include "metadataFlatbufferWriter.h"
#include "metadata_flat_knownFields.h"
#include <cstring>
#include <string>

class AICMetadataWriter;
[[maybe_unused]] void inline AICMetadata_dump([[maybe_unused]] const AICMetadata * a, [[maybe_unused]] FILE * b) {
  std::cerr<<"Dumping C Metadata is no longer supported"<<"\n";
}
[[maybe_unused]] static AICMetadata *MDR_readMetadata([[maybe_unused]] void *buff, [[maybe_unused]] int buffSizeInBytes,
                            [[maybe_unused]] char *errBuff,[[maybe_unused]] int errBuffSz) {
  std::cerr<<"Reading C Metadata is no longer supported"<<"\n";
  void *p = nullptr;
  return (AICMetadata *)p;
}

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
//START : STUB CODE SO COMPILER BUILDS
  template <class W>
  static auto
  flatbufValidateTranslateAicMetadata([[maybe_unused]] std::vector<uint8_t> &flatbuf) {
    std::cerr << "Error: flatbufValidateTranslateAicMetadata is deprecated." << std::endl;
    return W{}; // Return a default-constructed object of type W
  }
  template <class W>
  static const std::vector<uint8_t>
  flatbufValidateTranslateAicMetadataVector([[maybe_unused]] std::vector<uint8_t> &flatbuf){
    std::cerr << "Error: flatbufValidateTranslateAicMetadataVector is deprecated." << std::endl;
    return std::vector<uint8_t>{}; // Return an empty output variable
  }
//END: STUB CODE SO COMPILER BUILDS
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
// an AICMetadata.  Implemented by conversion as the conversion is unit
// tested.
//------------------------------------------------------------------------
inline auto dump(std::vector<uint8_t> &flatbuf_bytes) {
  std::string metadataError;
  auto metadata = metadata::FlatDecode::readMetadataFlatNativeCPP(
      flatbuf_bytes, metadataError);
  if (!metadataError.empty()) {
    throw std::runtime_error("Failed to parse metadata to dump it " +
                             metadataError);
  }
  return AICMetadata_dump(metadata);
}
inline auto dump(std::vector<uint8_t> &flatbuf_bytes,
                 const std::string &outputPath) {
  std::string metadataError;
  auto metadata = metadata::FlatDecode::readMetadataFlatNativeCPP(
      flatbuf_bytes, metadataError);
  if (!metadataError.empty()) {
    throw std::runtime_error("Failed to parse metadata to dump it " +
                             metadataError);
  }
  AICMetadata_dump(metadata, outputPath);
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
