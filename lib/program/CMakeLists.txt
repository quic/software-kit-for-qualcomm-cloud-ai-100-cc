include(FindProtobuf)

set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../networkdesc/proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ProgramConfig.proto)

get_filename_component(COMPILED_PROTO_INCLUDE_PARENT_DIR ${PROTO_HDRS} DIRECTORY)

message(STATUS "protobuf bindir ${protobuf_BINARY_DIR}")
message(STATUS "protobuf lib ${protobuf_LIBRARY}")
add_library(Program STATIC
   Program.cpp
   ProgramConfig.cpp
   QPCBuilder.cpp
   ${PROTO_SRCS})
target_link_libraries(Program PUBLIC AICMetadata AICMetadataWriter AICMetadataReader AICNetworkDescProto QAicQpc metadataFlatbufferWriter ExecContext_writer  Support LLVMSupport libprotobuf)
target_include_directories(Program
   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
   PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${COMPILED_PROTO_INCLUDE_PARENT_DIR} ${AIC_NETWORK_DESCRIPTION_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/..
   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../metadata/metadata-flatbuffer/include)
