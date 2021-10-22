cmake_minimum_required(VERSION 3.10)

project(efa-libfabric-bench)

SET (LIBFABRIC_INCLUDE_DIR "/opt/amazon/efa/include/")

add_custom_target(deploy ALL)

add_custom_command(TARGET deploy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/efa-bench/client.conf ${CMAKE_BINARY_DIR}/client.conf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/efa-bench/server.conf ${CMAKE_BINARY_DIR}/server.conf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/efa-bench/benchmark ${CMAKE_BINARY_DIR}/benchmark
)

add_subdirectory(libefa)
add_subdirectory(efa-bench)

add_dependencies(deploy benchmark)
