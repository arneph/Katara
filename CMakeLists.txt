cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE katara_h_files src/*.h)
file(GLOB_RECURSE katara_cc_files src/*.cc)

project(Katara)
include_directories(
	src
)
add_executable(katara ${katara_cc_files} ${katara_h_files})
add_definitions(-std=c++17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)