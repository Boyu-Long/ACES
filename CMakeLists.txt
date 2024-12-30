cmake_minimum_required(VERSION 3.10.2)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

project(ACES)

# include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sim/include/aces)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sim/include/)

# source file
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/sim/source DIR_SRCS)

# output file
set(OUTPATH  ../)
set(EXECUTABLE_OUTPUT_PATH ${OUTPATH})
add_executable(aces ${DIR_SRCS})


