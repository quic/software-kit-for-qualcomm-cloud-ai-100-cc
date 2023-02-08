include(GoogleTest)

add_executable(ToolchainToolsTest ToolchainToolsTest.cpp)
target_link_libraries(ToolchainToolsTest PUBLIC Toolchain gtest_main)
gtest_add_tests(TARGET ToolchainToolsTest)

add_custom_command(TARGET ToolchainToolsTest PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ToolchainToolsTest>/mock_tools
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/mock_tools $<TARGET_FILE_DIR:ToolchainToolsTest>/mock_tools)

add_executable(CompilerTests CompilerTests.cpp)
target_link_libraries(CompilerTests PUBLIC Toolchain gtest_main)
gtest_add_tests(TARGET CompilerTests)

add_executable(SupportTests StringListTests.cpp PathTests.cpp)
target_link_libraries(SupportTests PUBLIC Toolchain gtest_main)
gtest_add_tests(TARGET SupportTests)

add_executable(ProgramTests ProgramTests.cpp)
target_link_libraries(ProgramTests PUBLIC Program gtest_main)
gtest_add_tests(TARGET ProgramTests)

add_custom_command(TARGET ProgramTests PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_SOURCE_DIR}/test_program.json $<TARGET_FILE_DIR:ProgramTests>/test_program.json
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_SOURCE_DIR}/example_config.json $<TARGET_FILE_DIR:ProgramTests>/example_config.json
                   )

add_executable(QPCBuilderTests QPCBuilderTests.cpp)
target_link_libraries(QPCBuilderTests PUBLIC Program gtest_main)
gtest_add_tests(TARGET QPCBuilderTests)

add_custom_command(TARGET QPCBuilderTests PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_SOURCE_DIR}/qpc_segment_data.txt $<TARGET_FILE_DIR:QPCBuilderTests>/qpc_segment_data.txt)

add_executable(DriverTests DriverTests.cpp)
target_link_libraries(DriverTests PUBLIC gtest_main Driver)

gtest_add_tests(TARGET DriverTests)
