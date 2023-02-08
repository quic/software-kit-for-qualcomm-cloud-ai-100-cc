
add_library(Toolchain STATIC
   Tools.cpp
   ToolBase.cpp
   Compiler.cpp
   Linker.cpp
   ObjCopy.cpp
   Ar.cpp)
target_link_libraries(Toolchain PUBLIC Support)
target_include_directories(Toolchain INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
