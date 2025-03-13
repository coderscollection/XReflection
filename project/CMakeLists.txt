cmake_minimum_required(VERSION 3.10.2)
project (testproject)

set(PROJECT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_LIB_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

#default on ubuntu is fine
#set(CMAKE_C_COMPILER "/usr/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -DLinux -pthread -std=c++14")
MESSAGE( STATUS "COMPILE FLAG: " ${CMAKE_CXX_FLAGS} )

#reflection header only library - my work. 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/XReflection/Include/)

#and our program
add_subdirectory( XReflection )
