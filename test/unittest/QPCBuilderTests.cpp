// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cstring>
#include <fstream>
#include <gtest/gtest.h>

#include "program/QPCBuilder.h"

using namespace llvm;
using namespace qaic;

TEST(Program, QPCBuilder_AddSegment) {
  QPCBuilder builder;

  const char data1[] = {'t', 'e', 's', 't', '\0'};
  const int data2[] = {1, 2, 3, 4, 5, 6};
  StringRef data3 = "a quick fox jumped";

  builder.addSegment("data1", data1);
  builder.addSegment("data2", ArrayRef<int>{data2, sizeof(data2)}, 123);
  builder.addSegment("data3", data3);
  builder.addSegment("data1_with_offset", data1, 99);

  EXPECT_TRUE(builder.hasSegment("data1"));
  EXPECT_TRUE(builder.hasSegment("data2"));
  EXPECT_TRUE(builder.hasSegment("data3"));
  EXPECT_TRUE(builder.hasSegment("data1_with_offset"));
  EXPECT_FALSE(builder.hasSegment("badseg"));

  EXPECT_EQ(4, builder.getNumSegments());

  EXPECT_EQ(sizeof(data1), builder.getSegmentData("data1").size());
  EXPECT_EQ(0, builder.getSegmentOffset("data1"));
  EXPECT_EQ(0, std::memcmp(data1, builder.getSegmentData("data1").data(),
                           sizeof(data1)));

  EXPECT_EQ(sizeof(data2), builder.getSegmentData("data2").size());
  EXPECT_EQ(123, builder.getSegmentOffset("data2"));
  EXPECT_EQ(0, std::memcmp(data2, builder.getSegmentData("data2").data(),
                           sizeof(data2)));

  EXPECT_EQ(data3.size() + 1, builder.getSegmentData("data3").size());
  EXPECT_EQ(0, builder.getSegmentOffset("data3"));
  EXPECT_EQ(0, std::memcmp(data3.data(), builder.getSegmentData("data3").data(),
                           data3.size() + 1));

  EXPECT_EQ(sizeof(data1), builder.getSegmentData("data1_with_offset").size());
  EXPECT_EQ(99, builder.getSegmentOffset("data1_with_offset"));
  EXPECT_EQ(0, std::memcmp(data1,
                           builder.getSegmentData("data1_with_offset").data(),
                           sizeof(data1)));
}

TEST(Program, QPCBuilder_AddFileSegment) {
  QPCBuilder builder;

  StringRef refData = "test data from file 1234567890";
  ArrayRef<uint8_t> refDataArray{(const uint8_t *)refData.data(),
                                 refData.size()};

  ASSERT_TRUE(builder.addSegmentFromFile("seg0", "./qpc_segment_data.txt"));
  EXPECT_TRUE(builder.hasSegment("seg0"));
  EXPECT_EQ(0, builder.getSegmentOffset("seg0"));

  auto segData = builder.getSegmentData("seg0");
  EXPECT_EQ(
      0, std::memcmp(refDataArray.data(), segData.data(), refDataArray.size()));
  EXPECT_EQ(segData.size(), refDataArray.size());
}

TEST(Program, QPCBuilder_RemoveSegment) {
  QPCBuilder builder;

  const char data1[] = {'t', 'e', 's', 't', '\0'};
  builder.addSegment("data1", data1, 59);
  EXPECT_TRUE(builder.hasSegment("data1"));
  EXPECT_EQ(1, builder.getNumSegments());

  builder.removeSegment("data1");
  EXPECT_FALSE(builder.hasSegment("data1"));

  // Safe to call remove on non existing segment
  builder.removeSegment("no_segment");

  EXPECT_EQ(0, builder.getNumSegments());
}

TEST(Program, QPCBuilder_GetSegmentNonExisting) {
  QPCBuilder builder;
  EXPECT_FALSE(builder.hasSegment("foo"));
  EXPECT_EQ(0, builder.getSegmentOffset("foo"));

  ArrayRef<uint8_t> empty;
  EXPECT_EQ(empty, builder.getSegmentData("foo"));
}

TEST(Program, QPCBuilder_Reset) {
  QPCBuilder builder;

  const char data1[] = {'t', 'e', 's', 't', '\0'};
  const int data2[] = {1, 2, 3, 4, 5, 6};
  StringRef data3 = "a quick fox jumped";

  builder.addSegment("data1", data1);
  builder.addSegment("data2", ArrayRef<int>{data2, sizeof(data2)}, 123);
  builder.addSegment("data3", data3);
  builder.addSegment("data1_with_offset", data1, 99);

  EXPECT_TRUE(builder.hasSegment("data1"));
  EXPECT_TRUE(builder.hasSegment("data2"));
  EXPECT_TRUE(builder.hasSegment("data3"));
  EXPECT_TRUE(builder.hasSegment("data1_with_offset"));

  builder.reset();
  EXPECT_EQ(0, builder.getNumSegments());
}

TEST(Program, QPCBuilder_Finalize) {
  QPCBuilder builder;

  const char data1[] = {'t', 'e', 's', 't', '\0'};
  const int data2[] = {1, 2, 3, 4, 5, 6};
  StringRef data3 = "a quick fox jumped";

  builder.addSegment("data1", data1);
  builder.addSegment("data2", ArrayRef<int>{data2, sizeof(data2)}, 123);
  builder.addSegment("data3", data3);
  builder.addSegment("data1_with_offset", data1, 99);

  EXPECT_TRUE(builder.hasSegment("data1"));
  EXPECT_TRUE(builder.hasSegment("data2"));
  EXPECT_TRUE(builder.hasSegment("data3"));
  EXPECT_TRUE(builder.hasSegment("data1_with_offset"));

  destroyQpcHandle(builder.finalize());
  EXPECT_EQ(0, builder.getNumSegments());
}

TEST(Program, QPCBuilder_FinalizeToByteArray) {
  QPCBuilder builder;

  const char data1[] = {'t', 'e', 's', 't', '\0'};
  const int data2[] = {1, 2, 3, 4, 5, 6};
  StringRef data3 = "a quick fox jumped";

  builder.addSegment("data1", data1);
  builder.addSegment("data2", ArrayRef<int>{data2, sizeof(data2)}, 123);
  builder.addSegment("data3", data3);
  builder.addSegment("data1_with_offset", data1, 99);

  EXPECT_TRUE(builder.hasSegment("data1"));
  EXPECT_TRUE(builder.hasSegment("data2"));
  EXPECT_TRUE(builder.hasSegment("data3"));
  EXPECT_TRUE(builder.hasSegment("data1_with_offset"));

  auto array = builder.finalizeToByteArray();
  EXPECT_EQ(0, builder.getNumSegments());

  // Serialized data should be at least the size of the data since
  // we don't have any compression
  EXPECT_GT(array.size(), sizeof(data1) + sizeof(data2) + data3.size() + 1);
}

bool compareQpc(QAicQpc *srcQpc, QAicQpc *cloneQpc, bool ignoreHeader = false) {
  int count;

  if ((srcQpc == nullptr) || (cloneQpc == nullptr)) {
    return false;
  }

  if (ignoreHeader == false) {
    // Compare QpcHeader minus the base
    if (0 !=
        memcmp(&(srcQpc->hdr), &(cloneQpc->hdr), (sizeof(QpcHeader) - 8))) {
      fprintf(stderr, "QPC headers do not match\n");
      return false;
    }
  }

  if (srcQpc->numImages != cloneQpc->numImages) {
    fprintf(stderr, "Num images do not match\n");
    return false;
  }

  for (count = 0; count < cloneQpc->numImages; ++count) {
    QpcSegment *src = &(srcQpc->images[count]);
    QpcSegment *clone = &(cloneQpc->images[count]);

    if (src->size != clone->size) {
      fprintf(stderr, "Segment:%d sizes(src:%lu clone: %lu) do not match\n",
              count, src->size, clone->size);
      return false;
    }

    if (0 != strcmp(src->name, clone->name)) {
      fprintf(stderr, "Segment:%d names(src:%s clone: %s) do not match\n",
              count, src->name, clone->name);
      return false;
    }

    if (0 != memcmp(src->start, clone->start, src->size)) {
      fprintf(stderr, "Segment:%d contents(src:%s clone: %s) do not match\n",
              count, src->name, clone->name);
      return false;
    }
  }

  return true;
}

// Make sure we generate a valid QPC and can load it back
TEST(Program, QPCBuilder_RoundTripSerialize) {

  auto buildQPCAndReturnBuilder = []() {
    QPCBuilder builder;
    const char data1[] = {'t', 'e', 's', 't', '\0'};
    const int data2[] = {1, 2, 3, 4, 5, 6};
    StringRef data3 = "a quick fox jumped";

    builder.addSegment("data1", data1);
    builder.addSegment("data2", ArrayRef<int>{data2, sizeof(data2)}, 123);
    builder.addSegment("data3", data3);
    builder.addSegment("data1_with_offset", data1, 99);

    EXPECT_EQ(4, builder.getNumSegments());
    return builder;
  };

  auto builder1 = buildQPCAndReturnBuilder();
  auto builder2 = buildQPCAndReturnBuilder();

  auto serializedQPCBuf = builder1.finalizeToByteArray();
  QAicQpcHandle *expectedQPCHandle = builder2.finalize();

  QAicQpcHandle *testQPCHandle = nullptr;
  ASSERT_EQ(0, createQpcHandle(&testQPCHandle, CompressionType::SLOWPATH));
  ASSERT_EQ(0, buildFromByteArray(testQPCHandle, serializedQPCBuf.data()));

  QAicQpc *testQPC = nullptr;
  QAicQpc *expectedQPC = nullptr;
  getQpc(testQPCHandle, &testQPC);
  ASSERT_NE(nullptr, testQPC);
  getQpc(expectedQPCHandle, &expectedQPC);
  ASSERT_NE(nullptr, expectedQPC);

  EXPECT_TRUE(compareQpc(testQPC, expectedQPC, true));

  destroyQpcHandle(testQPCHandle);
  destroyQpcHandle(expectedQPCHandle);
}
