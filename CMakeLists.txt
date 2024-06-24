cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Clox LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")
add_executable(Clox ${SOURCES})
