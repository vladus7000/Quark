cmake_minimum_required(VERSION 3.8.0)

project(Universe)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE DEMO_INCLUDE *.hpp *.h)
file(GLOB_RECURSE DEMO_SOURCE *.cpp)

add_executable(Universe ${DEMO_INCLUDE} ${DEMO_SOURCE})
target_link_libraries(Universe AntiQuark)

include_directories(${ENGINE_ROOT})

include_directories(${ENGINE_ROOT}/Libraries/cereal/include/)
include_directories(${ENGINE_ROOT}/Libraries/entt/single_include/)
include_directories(${ENGINE_ROOT}/Libraries/glfw/include/)
include_directories(${ENGINE_ROOT}/Libraries/taskflow/)
include_directories(${ENGINE_ROOT}/Libraries/json/single_include/)
include_directories(${ENGINE_ROOT}/Libraries/glm/)
include_directories(${Vulkan_INCLUDE_DIR})

set_target_properties(
    Universe PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Data")