
add_custom_target(qaic-binutils)
add_custom_command(
        TARGET qaic-binutils
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/qaic-ar
                ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/qaic-nm
                ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/qaic-ranlib
                ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/qaic-size
                ${CMAKE_CURRENT_BINARY_DIR})

install(FILES qaic-ar DESTINATION exec PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
install(FILES qaic-nm DESTINATION exec PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
install(FILES qaic-ranlib DESTINATION exec PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
install(FILES qaic-size DESTINATION exec PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
