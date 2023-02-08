include(FetchContent)

#--------------------------------------------
#Get ElfIo Module
FetchContent_Declare(elfio
  GIT_REPOSITORY https://github.com/serge1/ELFIO.git
  GIT_TAG        Release_3.10
)
FetchContent_MakeAvailable(elfio)

#Expose interface library for Elfio Module
add_library(module-elfio INTERFACE)
target_include_directories(module-elfio INTERFACE
${elfio_SOURCE_DIR})


#--------------------------------------------
#Get Protobuf Module
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "Turn off tests")
FetchContent_Declare(protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        v3.11.4
  SOURCE_SUBDIR  cmake
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(protobuf)

#Expose interface library for Protobuf Module
add_library(module-protobuf INTERFACE)
target_include_directories(module-protobuf INTERFACE
${protobuf_SOURCE_DIR}/src)

#--------------------------------------------
#Get flatbuffers Module
FetchContent_Declare(flatbuffers
  GIT_REPOSITORY https://github.com/google/flatbuffers
  GIT_TAG        v2.0.8
)
FetchContent_MakeAvailable(flatbuffers)

#Expose interface library for FLATBUFFERS Module
add_library(module-flatbuffers
  ${flatbuffers_SOURCE_DIR}/src/idl_gen_text.cpp
  ${flatbuffers_SOURCE_DIR}/src/idl_parser.cpp
  ${flatbuffers_SOURCE_DIR}/src/util.cpp
)

target_include_directories(module-flatbuffers PUBLIC
        ${flatbuffers_SOURCE_DIR}/include)

#--------------------------------------------
#Get GTest
set(BUILD_GMOCK OFF CACHE INTERNAL "")
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)
FetchContent_MakeAvailable(googletest)
