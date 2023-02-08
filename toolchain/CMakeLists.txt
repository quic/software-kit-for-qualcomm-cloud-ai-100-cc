configure_file(qaic.cmake.in qaic.cmake COPYONLY)
set(QAIC_TOOLCHAIN_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR}/qaic.cmake)
install(FILES ${QAIC_TOOLCHAIN_BUILD_PATH} DESTINATION dev/cmake)

add_subdirectory(test)
