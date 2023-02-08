add_executable(qaic-objcopy qaic-objcopy.cpp)
target_link_libraries(qaic-objcopy Toolchain)

install(TARGETS qaic-objcopy RUNTIME DESTINATION exec)
