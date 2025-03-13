#xreflection makefile. (c) John Nessworthy. 

cmake_minimum_required(VERSION 3.10.3)
project (reflection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include/)
include_directories(${CMAKE_CURRENT_DIR}/Include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost_1_69_0)

#our test programs for the framework
add_subdirectory(Linux)
