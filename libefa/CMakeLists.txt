cmake_minimum_required(VERSION 3.10)

project(libefa)

SET (CMAKE_BUILD_TYPE Debug)

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")

SET (PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/libefa")

include_directories(${LIBFABRIC_INCLUDE_DIR})

file(GLOB libefa_src CONFIGURE_DEPENDS
${PROJECT_SOURCE_DIR}/*.h
${PROJECT_SOURCE_DIR}/*.cpp
${PROJECT_SOURCE_DIR}/ftlib/*.h
${PROJECT_SOURCE_DIR}/ftlib/*.c
)

ADD_LIBRARY( efa STATIC
${libefa_src} )
