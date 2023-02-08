configure_file(BarebonesTest.cpp.in BarebonesTest.cpp)
configure_file(BarebonesTestLib.cpp.in BarebonesTestLib.cpp)
configure_file(BarebonesConfig.json.in BarebonesConfig.json)

add_executable(CmakeToolchainTest CmakeToolchainTest.cpp)
add_dependencies(CmakeToolchainTest qaic-cc qaic-objcopy qaic-binutils qaicrt devRuntime)

target_compile_definitions(CmakeToolchainTest PRIVATE
   QAIC_TOOLCHAIN_BUILD_PATH="${QAIC_TOOLCHAIN_BUILD_PATH}"
   QAIC_METADATA_SOURCE_INCLUDE_PATH="${QAIC_METADATA_SOURCE_INCLUDE_PATH}"
   QAIC_RUNTIME_LIB_PATH="$<TARGET_FILE_DIR:qaicrt>")
target_link_libraries(CmakeToolchainTest PUBLIC gtest_main LLVMSupport)

gtest_add_tests(TARGET CmakeToolchainTest)
