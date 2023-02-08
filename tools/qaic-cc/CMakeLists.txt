add_executable(qaic-cc qaic-cc.cpp)
target_link_libraries(qaic-cc PUBLIC Driver)

install(TARGETS qaic-cc RUNTIME DESTINATION exec)
