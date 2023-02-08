// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_SUPPORT_STRINGLIST_H_
#define _QAIC_SUPPORT_STRINGLIST_H_

#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

namespace qaic {

/**
 * \brief A list of owned strings that can be converted to a list of StringRefs
 */
class StringList {
public:
  using StringVector = std::vector<std::string>;
  using iterator = StringVector::iterator;
  using const_iterator = StringVector::const_iterator;

  std::vector<llvm::StringRef> getRefList() const;

  void push_back(llvm::StringRef val) { strings_.push_back(val); }

  iterator begin() { return strings_.begin(); }
  iterator end() { return strings_.end(); }

  const_iterator begin() const { return strings_.begin(); }
  const_iterator end() const { return strings_.end(); }

  std::string &operator[](size_t idx) {
    assert(idx < strings_.size());
    return strings_[idx];
  }

  const std::string &operator[](size_t idx) const {
    assert(idx < strings_.size());
    return strings_[idx];
  }

  size_t size() const { return strings_.size(); }
  bool empty() const { return strings_.empty(); }
  void clear() { strings_.clear(); }

private:
  StringVector strings_;
};
} // namespace qaic

#endif
