cmake_minimum_required(VERSION 3.10)

project(Benchmark)

SET (PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

include_directories("/opt/amazon/efa/include/")
link_directories("/opt/amazon/efa/lib64/")

find_package(gflags REQUIRED)

add_executable(benchmark ${PROJECT_SOURCE_DIR}/benchmark.cpp)

TARGET_LINK_LIBRARIES( benchmark
pthread
fabric
gflags
)