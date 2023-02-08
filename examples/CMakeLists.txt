set(EXAMPLE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/barebones_app
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_io
  ${CMAKE_CURRENT_SOURCE_DIR}/multicast_example
  )

install(DIRECTORY ${EXAMPLE_DIRS}  DESTINATION examples/compute)
