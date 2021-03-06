cmake_minimum_required(VERSION 3.10)

project(efa-benchmark)

SET (CMAKE_BUILD_TYPE Debug)

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")

SET (CMAKE_CXX_STANDARD 17)

SET (PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

include_directories(${CMAKE_SOURCE_DIR}/libefa/src/)

include_directories(${LIBFABRIC_INCLUDE_DIR})
link_directories(${LIBFABRIC_LINK_DIR})

file(GLOB benchmark_SRC CONFIGURE_DEPENDS
${PROJECT_SOURCE_DIR}/*.h
${PROJECT_SOURCE_DIR}/*.cpp
)

add_executable(benchmark ${benchmark_SRC})

TARGET_LINK_LIBRARIES( benchmark
${CMAKE_BINARY_DIR}/libefa/libefa.a
)

TARGET_LINK_LIBRARIES( benchmark
pthread
fabric
gflags
)

add_dependencies(benchmark efa)