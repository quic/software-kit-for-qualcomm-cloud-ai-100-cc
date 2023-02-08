# Generate DriverOptions.inc from the tblgen input
add_custom_command(
    OUTPUT  DriverOptions.inc
    COMMAND ${LLVM_TOOLS_BINARY_DIR}/llvm-tblgen
               -gen-opt-parser-defs
               ${CMAKE_CURRENT_SOURCE_DIR}/DriverOptions.td 
               -o ${CMAKE_CURRENT_BINARY_DIR}/DriverOptions.inc
               -I ${LLVM_INCLUDE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/DriverOptions.td)

add_library(Driver STATIC 
   Driver.cpp
   DriverOptions.cpp
   DriverAction.cpp
   DriverContext.cpp
   ${CMAKE_CURRENT_BINARY_DIR}/DriverOptions.inc)
target_link_libraries(Driver PUBLIC Program Toolchain LLVMOption LLVMObject)
target_include_directories(Driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
