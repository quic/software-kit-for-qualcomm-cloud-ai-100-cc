cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(MulticastExampleProject C CXX)

# Make a static library for the QPC
add_qaic_library(MulticastLib STATIC MulticastLib.cpp)

# Create a QPC executable application
add_qaic_executable(Multicast multicast.json Multicast.cpp)
target_link_libraries(Multicast MulticastLib)
