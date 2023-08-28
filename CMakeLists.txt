cmake_minimum_required(VERSION 3.16)
project(VkTest)

find_package(Vulkan REQUIRED)

add_executable(vktest WIN32 vktest.cpp Device.cpp Pipeline.cpp Swapchain.cpp Renderer.cpp)
target_include_directories(vktest PUBLIC ${Vulkan_INCLUDE_DIR})
target_link_libraries(vktest ${Vulkan_LIBRARY})

add_custom_command(OUTPUT vert.spv COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/vert.spv DEPENDS shader.vert VERBATIM)
add_custom_target(vertshader DEPENDS vert.spv)

add_custom_command(OUTPUT frag.spv COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/frag.spv DEPENDS shader.frag VERBATIM)
add_custom_target(fragshader DEPENDS frag.spv)

add_dependencies(vktest vertshader fragshader)