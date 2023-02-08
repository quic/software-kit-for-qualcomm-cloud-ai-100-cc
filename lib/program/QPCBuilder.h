// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_QPCBUILDER_H_
#define _QAIC_QPCBUILDER_H_

#include <map>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include "networkdesc/qpc/inc/QAicQpc.h"

namespace qaic {

/**
 * @brief A utility for building QPCs
 */
class QPCBuilder {
public:
  QPCBuilder() = default;
  QPCBuilder(QPCBuilder &&builder) = default;

  /**
   * @brief Adds a segment to the QPC with the given name.
   *
   * @param name The name of the segment
   * @param ptr A pointer to the data to add to the segment
   * @param size The number of bytes
   * @param offset An offset for dynamic constant data
   */
  void addSegment(llvm::StringRef name, const uint8_t *ptr, size_t size,
                  size_t offset = 0);

  /**
   * @brief Adds a segment to the QPC with the given name.
   *
   * @param name The name of the segment
   * @param data A string containing data to add
   * @param offset An offset for dynamic constant data
   */
  void addSegment(llvm::StringRef name, llvm::StringRef data,
                  size_t offset = 0) {
    addSegment(name, (const uint8_t *)data.data(), data.size() + 1, offset);
  }

  /**
   * @brief Adds a segment to the QPC with the given name.
   *
   * @tparam T Data type for an ArrayRef
   * @param name The name of the segment
   * @param data An ArrayRef<T> of data to add
   * @param offset An offset for dynamic constant data.
   */
  template <class T>
  void addSegment(llvm::StringRef name, llvm::ArrayRef<T> data,
                  size_t offset = 0) {
    addSegment(name, (const uint8_t *)data.data(), data.size(), offset);
  }

  /**
   * @brief Adds a segment segment to the QPC with the given name. The constent
   * of the segment are loaded from a file.
   *
   * @param name The name of the segment.
   * @param filePath The path to a file to load into the segment.
   */
  bool addSegmentFromFile(llvm::StringRef name, llvm::StringRef filePath);

  /**
   * @brief Remove a segment with the given name.
   */
  void removeSegment(llvm::StringRef name);

  /**
   * @brief Returns true if a segment with the given name exists.
   */
  bool hasSegment(llvm::StringRef name) const;

  /**
   * @brief Returns the offset of a segment or 0 if it does not exist.
   */
  size_t getSegmentOffset(llvm::StringRef name) const;

  /**
   * @brief Returns the data of a segment or and empty array reference.
   */
  llvm::ArrayRef<uint8_t> getSegmentData(llvm::StringRef name) const;

  /**
   * @brief Get the number of segments added to this QPC
   */
  size_t getNumSegments() const { return segmentBufferMap_.size(); }

  /**
   * @brief Finalizes the QPC and returns it to the caller.
   *
   * The user is responsible for release the QPC handle by calling
   * destroyQpcHandle()
   *
   * @return QAicQpcHandle*
   */
  QAicQpcHandle *finalize();

  /**
   * @brief Finalizes the QPC and serializes it to a byte array.
   *
   * @return std::vector<uint8_t> The serialized QPC.
   */
  std::vector<uint8_t> finalizeToByteArray();

  /**
   * @brief Resets the builder to its default initial state.
   *
   * Any QPC under construction is release and not finalized.
   */
  void reset();

private:
  std::map<std::string, std::vector<uint8_t>> segmentBufferMap_;
  std::map<std::string, size_t> segmentOffsets_;
};
} // namespace qaic

#endif
