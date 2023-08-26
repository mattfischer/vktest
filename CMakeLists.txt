cmake_minimum_required(VERSION 3.16)
project(VkTest)

find_package(Vulkan REQUIRED)

add_executable(vktest WIN32 vktest.cpp)
target_include_directories(vktest PUBLIC ${Vulkan_INCLUDE_DIR})
target_link_libraries(vktest ${Vulkan_LIBRARY})