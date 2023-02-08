cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(SimpleIOExampleProject C CXX)

# Make a static library for the QPC
add_qaic_library(SimpleIOLib STATIC SimpleIOLib.cpp)

# Create a QPC executable application
add_qaic_executable(SimpleIO simpleio.json SimpleIO.cpp)
target_link_libraries(SimpleIO SimpleIOLib)
