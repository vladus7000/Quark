cmake_minimum_required(VERSION 3.8.0)

project(Universe)

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE DEMO_INCLUDE *.hpp *.h)
file(GLOB_RECURSE DEMO_SOURCE *.cpp)

add_library(Universe ${DEMO_INCLUDE} ${DEMO_SOURCE})
#target_link_libraries(Universe AntiQuark)

include_directories(${ENGINE_ROOT})
