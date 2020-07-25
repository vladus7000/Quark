cmake_minimum_required(VERSION 3.8.0)

project(Quark)

set(CMAKE_CXX_STANDARD 11)

set(ENGINE_ROOT "${CMAKE_SOURCE_DIR}/engine")
set(DEMO_ROOT "${CMAKE_SOURCE_DIR}/demo")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

add_subdirectory(Engine/)
add_subdirectory(Demo/)