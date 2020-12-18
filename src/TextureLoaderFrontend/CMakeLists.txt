cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(TextureLoaderFrontend)

set(TEXAS_INCLUDE_DIR "")
option(TEXTURE_LOADER_FRONTEND_USE_TEXAS "Add Texas as a backend")
if(TEXTURE_LOADER_FRONTEND_USE_TEXAS)
	message("==============================================")
	message("                    Texas                     ")
	message("==============================================")
	set(TEXAS_ENABLE_KTX_SAVE OFF CACHE BOOL "")
	set(TEXAS_ENABLE_PNG_READ OFF CACHE BOOL "")
	set(TEXAS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../external/Texas")
	add_subdirectory(${TEXAS_DIR})
	set_target_properties(Texas PROPERTIES FOLDER "Third Party")
	set(TEXAS_INCLUDE_DIR "${TEXAS_DIR}/include")
endif()

set(ASTC_CODEC_INCLUDE_DIR "")
option(TEXTURE_LOADER_FRONTEND_USE_TEXAS "Add Texas as a backend")
if(TEXTURE_LOADER_FRONTEND_USE_ASTC_CODEC)
	message("==============================================")
	message("                  astc-codec                  ")
	message("==============================================")
	set(OPTION_ASTC_TESTS OFF CACHE BOOL "")
	set(ASTCCODEC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../external/astc-codec")
	add_subdirectory(${ASTCCODEC_DIR})
	set_target_properties(astc_inspector_cli PROPERTIES FOLDER "Third Party/astc-codec")
	set_target_properties(astc_utils PROPERTIES FOLDER "Third Party/astc-codec")
	set_target_properties(astc-codec PROPERTIES FOLDER "Third Party/astc-codec")
	set_target_properties(footprint PROPERTIES FOLDER "Third Party/astc-codec")
	set(ASTC_CODEC_INCLUDE_DIR "${ASTCCODEC_DIR}/include")
endif()

add_library(TextureLoaderFrontend STATIC
	"${CMAKE_CURRENT_LIST_DIR}/TextureLoaderFrontend.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/TextureLoaderFrontend.hpp"
	"${CMAKE_CURRENT_LIST_DIR}/../third_party/stb_image.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/../third_party/stb_image.hpp"
)

if(TEXTURE_LOADER_FRONTEND_USE_TEXAS)
	target_link_libraries(TextureLoaderFrontend Texas)
endif()

if(TEXTURE_LOADER_FRONTEND_USE_ASTC_CODEC)
	target_link_libraries(TextureLoaderFrontend astc-codec)
endif()
