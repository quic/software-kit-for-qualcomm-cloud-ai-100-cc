// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_PROGRAMDESC_H_
#define _QAIC_PROGRAMDESC_H_

#include "../../runtime/lib/BufferDesc.h"
#include "../../runtime/lib/SerializedProgramDesc.h"
#include <fstream>
#include <iostream>
#include <vector>

namespace qaic {

uint32_t getTypeSize(aicnwdesc::dataType type) {
  uint32_t typesize;
  switch (type) {
  case aicnwdesc::FloatTy: // 32-bit float type (float)
    typesize = sizeof(float);
    break;
  case aicnwdesc::Float16Ty: // 16-bit float type (half, fp16)
    typesize = sizeof(__fp16);
    break;
  case aicnwdesc::Int8QTy: // 8-bit quantized type (int8_t)
    typesize = sizeof(int8_t);
    break;
  case aicnwdesc::UInt8QTy: // unsigned 8-bit quantized type (uint8_t)
    typesize = sizeof(uint8_t);
    break;
  case aicnwdesc::Int16QTy: // 16-bit quantized type (int16_t)
    typesize = sizeof(int16_t);
    break;
  case aicnwdesc::Int32QTy: // 32-bit quantized type (int32_t)
    typesize = sizeof(int32_t);
    break;
  case aicnwdesc::Int32ITy: // 32-bit index type (int32_t)
    typesize = sizeof(int32_t);
    break;
  case aicnwdesc::Int64ITy: // 64-bit index type (int64_t)
    typesize = sizeof(int64_t);
    break;
  case aicnwdesc::Int8Ty: // 8-bit type (int8_t)
    typesize = sizeof(int8_t);
    break;
  default:
    llvm::errs() << "Config Error: Unknown data type in IODescription: "
                 << dataType_Name(type) << "\n";
    exit(-1);
  }
  return typesize;
}

uint32_t getIOSize(aicnwdesc::IODescription io) {
  if (io.dims_size() <= 0) {
    llvm::errs() << "Config Error: Buffers must have non-zero dims\n";
    exit(-1);
  }
  int dims = io.dims(0);
  for (int i = 1; i < io.dims_size(); i++)
    dims *= io.dims(i);

  uint32_t typesize = getTypeSize(io.type());

  if (dims == 0) {
    llvm::errs() << "Config Error: Buffers must have non-zero dims\n";
    exit(-1);
  }
  return dims * typesize;
}

class ProgramDesc {
private:
  uint16_t exitDB_;
  uint32_t numThreads_{0};
  uint16_t numInputBuffs_{0};
  uint16_t numOutputBuffs_{0};
  uint16_t numInternalBuffs_{0};
  uint16_t inputSem_{0};
  uint16_t outputSem_{0};
  uint16_t hasInputsMask_{0};
  uint16_t hasOutputsMask_{0};
  uint32_t udmaDescBuffNum_{0};
  uint32_t udmaDummyStartDescOffset_{0};

  std::vector<BufferDesc_t> buffers_;

public:
  ProgramDesc(uint16_t exitDB, uint16_t inputSem, uint16_t outputSem,
              uint32_t numThreads)
      : exitDB_(exitDB), inputSem_(inputSem), outputSem_(outputSem),
        numThreads_(numThreads){};

  void addBuffer(const aicnwdesc::IODescription &desc, uint16_t waitDBNum,
                 uint16_t ioDBNum, uint32_t waitDBVal, uint32_t ioDBVal,
                 uint16_t ioMCID, uint16_t ioDBMCID, uint16_t buffMCID,
                 uint16_t nspMask, usageType_t usage, bool allowPartial) {
    memLoc_t location;
    switch (desc.dest()) {
    case aicnwdesc::L2TCM:
      location = qaic::L2TCM;
      break;
    case aicnwdesc::VTCM:
      location = qaic::VTCM;
      break;
    case aicnwdesc::DDR:
      location = qaic::DDR;
      break;
    default:
      llvm::errs() << "Invalid location " << std::to_string(desc.dest())
                   << "\n";
      exit(-1);
    }
    BufferDesc_t buffer = {.location = location,
                           .offset = desc.devoffset() + desc.baseaddroffset(),
                           .size = getIOSize(desc),
                           .waitDBNum = waitDBNum,
                           .ioDBNum = ioDBNum,
                           .waitDBVal = waitDBVal,
                           .ioDBVal = ioDBVal,
                           .ioMCID = ioMCID,
                           .ioDBMCID = ioDBMCID,
                           .buffMCID = buffMCID,
                           .nspMask = nspMask,
                           .usage = usage,
                           .allowPartial = allowPartial};
    buffers_.push_back(std::move(buffer));
    if (usage == USAGE_INPUT) {
      numInputBuffs_++;
      hasInputsMask_ |= nspMask;
    } else if (usage == USAGE_OUTPUT) {
      numOutputBuffs_++;
      hasOutputsMask_ |= nspMask;
    } else if (usage == USAGE_INTERNAL)
      numInternalBuffs_++;
  }

  void addUDMADescBuffer(aicnwdesc::IODescription &udmaBuff,
                         uint32_t udmaDummyStartDescOffset, uint16_t nspMask) {
    udmaDummyStartDescOffset_ = udmaDummyStartDescOffset;
    udmaDescBuffNum_ = buffers_.size();
    addBuffer(udmaBuff, 0, 0, 0, 0, 0, 0, 0, nspMask, USAGE_INTERNAL, 0);
  }

  uint32_t serialize(std::ostream &f) {
    if (SERIALIZED_PROGRAMDESC_VERSION == 1) {
      uint16_t numBuffs = buffers_.size();
      uint32_t buffOffset = sizeof(SerializedProgramDesc_t);
      uint32_t size = buffOffset + (numBuffs * sizeof(BufferDesc_t));

      f.write((char *)&SERIALIZED_PROGRAMDESC_VERSION,
              sizeof(SERIALIZED_PROGRAMDESC_VERSION));
      f.write((char *)&exitDB_, sizeof(exitDB_));
      f.write((char *)&size, sizeof(size));
      f.write((char *)&numThreads_, sizeof(numThreads_));
      f.write((char *)&numBuffs, sizeof(numBuffs));
      f.write((char *)&numInputBuffs_, sizeof(numInputBuffs_));
      f.write((char *)&numOutputBuffs_, sizeof(numOutputBuffs_));
      f.write((char *)&numInternalBuffs_, sizeof(numInternalBuffs_));
      f.write((char *)&inputSem_, sizeof(inputSem_));
      f.write((char *)&outputSem_, sizeof(outputSem_));
      f.write((char *)&hasInputsMask_, sizeof(hasInputsMask_));
      f.write((char *)&hasOutputsMask_, sizeof(hasOutputsMask_));
      f.write((char *)&buffOffset, sizeof(buffOffset));
      f.write((char *)&udmaDescBuffNum_, sizeof(udmaDescBuffNum_));
      f.write((char *)&udmaDummyStartDescOffset_,
              sizeof(udmaDummyStartDescOffset_));

      for (auto &buff : buffers_) {
        f.write((char *)&buff, sizeof(buff));
      }
      return size;
    } else {
      llvm::errs() << "Unknown SERIALIZED_PROGRAMDESC_VERSION"
                   << std::to_string(SERIALIZED_PROGRAMDESC_VERSION) << "\n";
      exit(-1);
    }
  }
};

} // namespace qaic
#endif
