cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(BarebonesAppExampleProject C CXX)

# Make a static library for the QPC
add_qaic_library(BarebonesAppLib BarebonesAppLib.cpp)

# Create a QPC executable application
add_qaic_executable(BarebonesApp barebones.json BarebonesApp.cpp)
target_link_libraries(BarebonesApp BarebonesAppLib)
