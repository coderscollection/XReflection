cmake_minimum_required(VERSION 2.8)
project(ReflectionTest)

include_directories( 
    ${REFLECTION_DIRECTORY}/Include
    ${BOOST_DIRECTORY}
)

file(GLOB SOURCES "*.cpp")

add_executable (${PROJECT_NAME} ${SOURCES})

