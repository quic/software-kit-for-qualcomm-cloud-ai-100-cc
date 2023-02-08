add_library(Support STATIC
   Debug.cpp
   StringList.cpp
   Path.cpp)
target_link_libraries(Support PUBLIC LLVMSupport)
target_include_directories(Support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
