project(AICExecContext)

add_library(AICExecContext INTERFACE)

if (CUSTOM_FLATC_EXEC)
  set(GENERATED_FLATBUFFER_PATH_FOR_METADATA_EXEC_CONTEXT ${CMAKE_CURRENT_BINARY_DIR})
  flatbuild(${GENERATED_FLATBUFFER_PATH_FOR_METADATA_EXEC_CONTEXT} execContext_generated.h execContext.fbs ${CUSTOM_FLATC_EXEC} execContext_generated)
  add_dependencies(AICExecContext execContext_generated)
else()
  set(GENERATED_FLATBUFFER_PATH_FOR_METADATA_EXEC_CONTEXT ${CMAKE_CURRENT_SOURCE_DIR}/deps-generated/flatbuffers)
endif()

target_include_directories(AICExecContext
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-generated
        ${GENERATED_FLATBUFFER_PATH_FOR_METADATA_EXEC_CONTEXT}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        )

target_link_libraries(AICExecContext
        INTERFACE
        flatbuffers
        )

project(ExecContext_writer)

add_library(ExecContext_writer
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/execContextWriter.hpp
)

target_include_directories(ExecContext_writer
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-generated
)

target_link_libraries(ExecContext_writer
    INTERFACE
        flatbuffers
        AICExecContext
)

add_library(ExecContextQSM_firmwareDeps INTERFACE)
target_include_directories(ExecContextQSM_firmwareDeps
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/../metadata-flatbuffer/test/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/qsm/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nsp/
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-generated/firmware/
        )
