// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <fstream>
#include <gtest/gtest.h>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"

class CmakeToolchain : public ::testing::Test {
protected:
  int runCmake(llvm::ArrayRef<llvm::StringRef> args) {
    llvm::ErrorOr<std::string> exe = llvm::sys::findProgramByName("cmake");
    EXPECT_TRUE(exe) << "Can not find cmake in PATH!";
    if (!exe)
      return -1;

    std::vector<llvm::StringRef> formedArgs;
    formedArgs.push_back(exe.get());
    for (auto a : args)
      formedArgs.push_back(a);

    formedArgs.push_back("-DCMAKE_VERBOSE_MAKEFILE=1");

    std::string errMsg;
    int rc =
        llvm::sys::ExecuteAndWait(exe.get(), formedArgs, {}, {}, 0, 0, &errMsg);
    EXPECT_EQ(0, rc) << errMsg << "\n";
    return rc;
  }

  int runBuild(llvm::StringRef path) {
    llvm::ErrorOr<std::string> exe = llvm::sys::findProgramByName("cmake");
    EXPECT_TRUE(exe) << "Can not find cmake in PATH!";
    if (!exe)
      return -1;

    std::vector<llvm::StringRef> formedArgs;
    formedArgs.push_back(exe.get());
    formedArgs.push_back("--build");
    formedArgs.push_back(path);

    std::string errMsg;
    int rc =
        llvm::sys::ExecuteAndWait(exe.get(), formedArgs, {}, {}, 0, 0, &errMsg);
    EXPECT_EQ(0, rc) << errMsg << "\n";
    return rc;
  }

  void SetUp() override {
    std::string path = "./";
    path += ::testing::UnitTest::GetInstance()->current_test_info()->name();
    path += "/build";

    if (llvm::sys::fs::exists(path)) {
      llvm::dbgs() << "Removing old path... " << path << "\n";
      auto ec = llvm::sys::fs::remove_directories(path);
      ASSERT_FALSE(ec) << "Failed to remote old test directory " << path << "\n"
                       << ec.message() << "\n";
    }

    auto ec = llvm::sys::fs::create_directories(path);
    ASSERT_FALSE(ec) << "Failed to create test directory " << path << "\n"
                     << ec.message() << "\n";

    ec = llvm::sys::fs::set_current_path(path);
    ASSERT_FALSE(ec) << "Failed to set current working directory.";
    llvm::dbgs() << "Set path to: " << path << "\n";
  }
};

TEST_F(CmakeToolchain, CmakeCheck) { ASSERT_EQ(0, runCmake({"--version"})); }

/// Verifies that we can load the toolchain file, this invokes qaic-cc
TEST_F(CmakeToolchain, SanityToolchainFileLoads) {
  std::ofstream os{"../CMakeLists.txt"};
  os << "cmake_minimum_required(VERSION 3.13 FATAL_ERROR)\n";
  os << "project(SanityToolchainFileLoads)\n";
  os << "message(STATUS ${CMAKE_CURRENT_BINARY_DIR})";
  os.close();

  std::string toolchainFile = "-DCMAKE_TOOLCHAIN_FILE=";
  toolchainFile += QAIC_TOOLCHAIN_BUILD_PATH;

  ASSERT_EQ(0, runCmake({"..", toolchainFile}));
}

/// Verifies that we can compile an "empty" application
TEST_F(CmakeToolchain, SanityToolchainBuildExeNoMetadata) {
  std::ofstream os{"../CMakeLists.txt"};
  os << "cmake_minimum_required(VERSION 3.13 FATAL_ERROR)\n";
  os << "project(SanityToolchainBuild)\n";
  os << "add_executable(BarebonesTest BarebonesTest.cpp)\n";
  os << "target_include_directories(BarebonesTest PRIVATE "
     << QAIC_METADATA_SOURCE_INCLUDE_PATH << ")\n";
  os << "set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} "
        "-no-defaultlib-paths -L"
     << QAIC_RUNTIME_LIB_PATH << "\")";
  os.close();

  auto ec = llvm::sys::fs::copy_file("../../BarebonesTest.cpp",
                                     "../BarebonesTest.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTest.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesConfig.json",
                                "../BarebonesConfig.json");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesConfig.json\n";

  std::string toolchainFile = "-DCMAKE_TOOLCHAIN_FILE=";
  toolchainFile += QAIC_TOOLCHAIN_BUILD_PATH;

  ASSERT_EQ(0, runCmake({"..", toolchainFile}));
  ASSERT_EQ(0, runBuild("."));
}

/// Verifies that we can compile an "empty" application
TEST_F(CmakeToolchain, SanityToolchainBuildExeWithMetadata) {
  std::ofstream os{"../CMakeLists.txt"};
  os << "cmake_minimum_required(VERSION 3.13 FATAL_ERROR)\n";
  os << "project(SanityToolchainBuild)\n";
  os << "add_executable(BarebonesTest BarebonesTest.cpp)\n";
  os << "target_include_directories(BarebonesTest PRIVATE "
     << QAIC_METADATA_SOURCE_INCLUDE_PATH << ")\n";
  os << "set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} "
        "-no-defaultlib-paths -L"
     << QAIC_RUNTIME_LIB_PATH << "\")";
  os.close();

  auto ec = llvm::sys::fs::copy_file("../../BarebonesTest.cpp",
                                     "../BarebonesTest.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTest.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesConfig.json",
                                "../BarebonesConfig.json");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesConfig.json\n";

  std::string toolchainFile = "-DCMAKE_TOOLCHAIN_FILE=";
  toolchainFile += QAIC_TOOLCHAIN_BUILD_PATH;

  ASSERT_EQ(0, runCmake({"..", toolchainFile}));
  ASSERT_EQ(0, runBuild("."));
}

/// Verifies that we can compile an "empty" application into qpc
TEST_F(CmakeToolchain, SanityToolchainBuildQpc) {
  std::ofstream os{"../CMakeLists.txt"};
  os << "cmake_minimum_required(VERSION 3.13 FATAL_ERROR)\n";
  os << "project(SanityToolchainBuild)\n";
  os << "add_qaic_executable(BarebonesTest "
        "${CMAKE_CURRENT_SOURCE_DIR}/BarebonesConfig.json BarebonesTest.cpp)\n";
  os << "target_include_directories(BarebonesTest PRIVATE "
     << QAIC_METADATA_SOURCE_INCLUDE_PATH << ")\n";
  os << "set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} "
        "-no-defaultlib-paths -L"
     << QAIC_RUNTIME_LIB_PATH << "\")";
  os.close();

  auto ec = llvm::sys::fs::copy_file("../../BarebonesTest.cpp",
                                     "../BarebonesTest.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTest.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesConfig.json",
                                "../BarebonesConfig.json");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesConfig.json\n";

  std::string toolchainFile = "-DCMAKE_TOOLCHAIN_FILE=";
  toolchainFile += QAIC_TOOLCHAIN_BUILD_PATH;

  ASSERT_EQ(0, runCmake({"..", toolchainFile}));
  ASSERT_EQ(0, runBuild("."));
}

/// Verifies that we can compile with multiple files given to
/// add_qaic_executable
TEST_F(CmakeToolchain, SanityToolchainBuildQpcMultipleSourceFiles) {
  std::ofstream os{"../CMakeLists.txt"};
  os << "cmake_minimum_required(VERSION 3.13 FATAL_ERROR)\n";
  os << "project(SanityToolchainBuildQpcMultipleSourceFiles)\n";
  os << "add_qaic_executable(BarebonesTest "
        "${CMAKE_CURRENT_SOURCE_DIR}/BarebonesConfig.json BarebonesTest.cpp "
        "BarebonesTestLib.cpp)\n";
  os << "target_include_directories(BarebonesTest PRIVATE "
     << QAIC_METADATA_SOURCE_INCLUDE_PATH << ")\n";
  os << "set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} "
        "-no-defaultlib-paths -L"
     << QAIC_RUNTIME_LIB_PATH << "\")";
  os.close();

  auto ec = llvm::sys::fs::copy_file("../../BarebonesTest.cpp",
                                     "../BarebonesTest.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTest.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesTestLib.cpp",
                                "../BarebonesTestLib.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTestLib.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesConfig.json",
                                "../BarebonesConfig.json");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesConfig.json\n";

  std::string toolchainFile = "-DCMAKE_TOOLCHAIN_FILE=";
  toolchainFile += QAIC_TOOLCHAIN_BUILD_PATH;

  ASSERT_EQ(0, runCmake({"..", toolchainFile}));
  ASSERT_EQ(0, runBuild("."));
}

TEST_F(CmakeToolchain, SanityToolchainBuildQpcSetCxxStd) {
  std::ofstream os{"../CMakeLists.txt"};
  os << "cmake_minimum_required(VERSION 3.13 FATAL_ERROR)\n";
  os << "project(SanityToolchainBuildQpcMultipleSourceFiles)\n";
  os << "add_qaic_executable(BarebonesTest "
        "${CMAKE_CURRENT_SOURCE_DIR}/BarebonesConfig.json BarebonesTest.cpp "
        "BarebonesTestLib.cpp)\n";
  os << "target_include_directories(BarebonesTest PRIVATE "
     << QAIC_METADATA_SOURCE_INCLUDE_PATH << ")\n";
  os << "target_compile_options(BarebonesTest PUBLIC -std=c++14)\n";
  os << "set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} "
        "-no-defaultlib-paths -L"
     << QAIC_RUNTIME_LIB_PATH << "\")";
  os.close();

  auto ec = llvm::sys::fs::copy_file("../../BarebonesTest.cpp",
                                     "../BarebonesTest.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTest.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesTestLib.cpp",
                                "../BarebonesTestLib.cpp");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesTestLib.cpp\n";

  ec = llvm::sys::fs::copy_file("../../BarebonesConfig.json",
                                "../BarebonesConfig.json");
  ASSERT_FALSE(ec) << "Failed to copy BarebonesConfig.json\n";

  std::string toolchainFile = "-DCMAKE_TOOLCHAIN_FILE=";
  toolchainFile += QAIC_TOOLCHAIN_BUILD_PATH;

  ASSERT_EQ(0, runCmake({"..", toolchainFile}));
  ASSERT_EQ(0, runBuild("."));
}
