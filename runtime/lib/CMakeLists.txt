set(HEXAGON_TOOLS_BIN ${HEXAGON_TOOLS_DIR}/bin)

set(IR_FLAGS -g0 -O2 -fvisibility=default -Wall -Wextra -Werror -Wglobal-constructors -Wno-pass-failed -Wno-frame-address)
# -mhmx is not available in the open source toolchain.  If it becomes available, add it here
set(HEXAGON_IR_FLAGS ${IR_FLAGS} -mv68 -mhvx=v68 --target=hexagon-unknown-elf)
set(HEXAGON_CXX_FLAGS -nostdinc++ -fno-exceptions -B ${HEXAGON_TOOLS_DIR}/target -Wglobal-constructors)

set(RUNTIME_SRCS
  _qaic_start.cpp
  SerializedProgramDesc.cpp
  BufferDesc.cpp
  NSPContext.cpp
  Exit.cpp
  ComputeAPI.cpp)

# Compile the HW target runtime
add_library(qaicrt STATIC ${RUNTIME_SRCS})
set_target_properties(qaicrt
                        PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        CXX_STANDARD 11
                        CXX_STANDARD_REQUIRED YES
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        OUTPUT_NAME libqaicrt.a
                        PREFIX ""
                        SUFFIX ""
                        RULE_LAUNCH_COMPILE "${HEXAGON_TOOLS_BIN}/clang++ <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE> #")
target_compile_options(qaicrt PRIVATE ${HEXAGON_IR_FLAGS} ${HEXAGON_CXX_FLAGS})
target_include_directories(qaicrt PUBLIC ${QAIC_METADATA_SOURCE_INCLUDE_PATH})
target_compile_definitions(qaicrt PRIVATE)
install(TARGETS qaicrt DESTINATION dev/lib/x86_64/compute)

install(FILES BufferDesc.h Exit.h ComputeAPI.h DESTINATION dev/inc/compute)

# Compile libdev
add_library(devRuntime
  libdev/os_hexagon.cpp
  libdev/os_common.cpp
  libdev/libdev_interface.cpp
  libdev/libdev_udma.cpp)
set_target_properties(devRuntime
                      PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        CXX_STANDARD 11
                        CXX_STANDARD_REQUIRED YES
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        RULE_LAUNCH_COMPILE "${HEXAGON_TOOLS_BIN}/clang++ <DEFINES> <INCLUDES> <FLAGS> -fno-sanitize=all -o <OBJECT> -c <SOURCE> #")
target_compile_definitions(devRuntime PRIVATE)
target_compile_options(devRuntime
                       PRIVATE
                         -Wno-c99-designator
                         ${HEXAGON_IR_FLAGS}
                         ${HEXAGON_CXX_FLAGS})
target_include_directories(devRuntime PUBLIC ${QAIC_METADATA_SOURCE_INCLUDE_PATH})

install(TARGETS devRuntime DESTINATION dev/lib/x86_64/compute)
