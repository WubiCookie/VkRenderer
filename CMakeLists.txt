cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VkRenderer)

add_subdirectory("src/TextureLoaderFrontend")
add_subdirectory("src/VkRenderer")

add_compile_definitions(_AMD64_)
