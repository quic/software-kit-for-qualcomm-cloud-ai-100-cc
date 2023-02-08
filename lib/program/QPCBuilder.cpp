// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "support/Debug.h"
#include "llvm/ADT/SmallVector.h"
#include <fstream>

#include "QPCBuilder.h"

using namespace qaic;

#define DEBUG_TYPE "program"

void QPCBuilder::addSegment(llvm::StringRef name, const uint8_t *ptr,
                            size_t size, size_t offset) {
  std::vector<uint8_t> vec(ptr, ptr + size);
  segmentBufferMap_.emplace(name, std::move(vec));
  segmentOffsets_.emplace(name, offset);
  QAIC_DEBUG_STREAM("adding section \"" << name << "\" (size = " << size
                                        << ") to QPC.\n");
}

bool QPCBuilder::addSegmentFromFile(llvm::StringRef name,
                                    llvm::StringRef filePath) {

  std::string fname = filePath.str();
  std::ifstream ifs(fname, std::ios::binary | std::ios::ate);
  if (!ifs) {
    llvm::errs() << "Failed to open " << fname << " for reading.\n";
    return false;
  }

  std::ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  segmentBufferMap_[name].resize(fileSize);
  ifs.read((char *)&segmentBufferMap_[name][0], fileSize);

  // if we fail to read all bytes then error out and don't add the segment
  if (ifs.gcount() != fileSize) {
    segmentBufferMap_.erase(name);
    return false;
  }

  QAIC_DEBUG_STREAM("adding section \""
                    << name << "\" (size = " << segmentBufferMap_[name].size()
                    << ") to QPC.\n");
  size_t offset;
  if (name.equals("constants.bin")) {
    offset = fileSize;
  } else {
    offset = 0;
  }
  segmentOffsets_.emplace(name, offset);
  return true;
}

void QPCBuilder::removeSegment(llvm::StringRef name) {
  segmentBufferMap_.erase(name);
  segmentOffsets_.erase(name);
}

bool QPCBuilder::hasSegment(llvm::StringRef name) const {
  bool has = (segmentBufferMap_.count(name) == 1);
  assert(has == (segmentOffsets_.count(name) == 1));
  return has;
}

size_t QPCBuilder::getSegmentOffset(llvm::StringRef name) const {
  auto itr = segmentOffsets_.find(name);
  if (itr == segmentOffsets_.end())
    return 0;
  return itr->second;
}

llvm::ArrayRef<uint8_t> QPCBuilder::getSegmentData(llvm::StringRef name) const {
  auto itr = segmentBufferMap_.find(name);
  if (itr == segmentBufferMap_.end())
    return {};
  return {itr->second.data(), itr->second.size()};
}

QAicQpcHandle *QPCBuilder::finalize() {

  llvm::SmallVector<QpcSegment, 8> qpcSegments;

  // Create the QpcSegments from the buffer map
  for (auto &kvp : segmentBufferMap_) {
    size_t offset = segmentOffsets_[kvp.first];
    qpcSegments.emplace_back(kvp.second.size(),
                             offset,
                             (char *)kvp.first.c_str(),
                             &kvp.second[0]);
  }

  QAicQpcHandle *qpcHandle = nullptr;
  int res = createQpcHandle(&qpcHandle, CompressionType::SLOWPATH);
  assert(res == 0 && "failed to create QPC handle");
  (void)res;

  res = buildFromSegments(qpcHandle, qpcSegments.data(), qpcSegments.size());
  if (res != 0) {
    llvm::errs() << "Failed to build QPC from Segments: " << res << "\n";
    destroyQpcHandle(qpcHandle);
    return {};
  }

  // Remove the buffer copies now that we have commited them to the QPC.
  segmentBufferMap_.clear();
  segmentOffsets_.clear();

  return qpcHandle;
}

std::vector<uint8_t> QPCBuilder::finalizeToByteArray() {
  QAicQpcHandle *handle = finalize();
  assert(handle != nullptr);

  uint8_t *serializedQpc = nullptr;
  size_t serializedQpcSize = 0;
  int rc = getSerializedQpc(handle, &serializedQpc, &serializedQpcSize);
  if (rc != 0) {
    llvm::errs() << "Failed to serialize QPC: " << rc << "\n";
    destroyQpcHandle(handle);
    return {};
  }

  // serializedQPC is only valid while its handle is valid, so need to copy it
  std::vector<uint8_t> buf(serializedQpcSize);
  if (!copyQpcBuffer(buf.data(), serializedQpc, serializedQpcSize)) {
    llvm::errs() << "Failed to copy serialize QPC: " << rc << "\n";
    destroyQpcHandle(handle);
    return {};
  }
  destroyQpcHandle(handle);
  return buf;
}

void QPCBuilder::reset() {
  segmentBufferMap_.clear();
  segmentOffsets_.clear();
}
