cmake_minimum_required(VERSION 3.15)
project(Lab1)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(. DIR_SRCS)
add_executable(Test ${DIR_SRCS})