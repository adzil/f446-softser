cmake_minimum_required(VERSION 3.5)
project(mac_software)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0")

file(GLOB SOURCE_FILES Src/*.c)
file(GLOB HEADER_FILES Inc/*.h)

include_directories(Inc)

add_executable(mac_software_launch ${SOURCE_FILES} ${HEADER_FILES})