cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
project(qaic-compute C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(HEXAGON_TOOLS_DIR "" CACHE STRING "Directory containing released hexagon toolchain")

string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
if( NOT uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG" )
  option(QAIC_ENABLE_ASSERTIONS "Enable assertions" OFF)
else()
  option(QAIC_ENABLE_ASSERTIONS "Enable assertions" ON)
endif()

if( QAIC_ENABLE_ASSERTIONS )
  message(STATUS "QAIC_ENABLE_ASSERTIONS=1 assertions are enabled!")
  # MSVC doesn't like _DEBUG on release builds. See PR 4379.
  if( NOT MSVC )
    add_definitions( -D_DEBUG )
  endif()
  # On non-Debug builds cmake automatically defines NDEBUG, so we
  # explicitly undefine it:
  if( NOT uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG" )
    add_definitions( -UNDEBUG )
    # Also remove /D NDEBUG to avoid MSVC warnings about conflicting defines.
    foreach (flags_var_to_scrub
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL)
      string (REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " "
        "${flags_var_to_scrub}" "${${flags_var_to_scrub}}")
    endforeach()
  endif()
endif()

find_package(LLVM CONFIG)
if(NOT LLVM_FOUND OR LLVM_VERSION VERSION_LESS 7.0)
  message(SEND_ERROR "LLVM >= 7.0 is required to build the QAIC toolchain.")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

enable_testing()

set(QAIC_METADATA_SOURCE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/metadata/metadata-common/inc)
set(QAIC_NETWORKDESC_SOURCE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/networkdesc/include)

add_subdirectory(thirdparty-modules)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)
add_subdirectory(runtime)

# This should come after any other lib or tool
add_subdirectory(toolchain)
add_subdirectory(examples)

# Install the SDK README.md
install(FILES README.md docs/QAICProgramConfig.txt DESTINATION .)
