cmake_minimum_required(VERSION 3.8)
project(triag)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES triangulation_skeleton.cpp)

#SET(CMAKE_CXX_FLAGS "-fno-stack-protector")

add_executable(triag ${SOURCE_FILES})