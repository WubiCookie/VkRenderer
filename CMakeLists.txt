cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY FOLDER "Third Party")

project(VkRenderer)

unset(PROJECTS_BINARIES_OUTPUT_DIR)

find_package(Vulkan REQUIRED)

message("==============================================")
message("                     GLFW                     ")
message("==============================================")
SET(BUILD_SHARED_LIBS OFF)
SET(GLFW_BUILD_DOCS OFF)
SET(GLFW_BUILD_EXAMPLES OFF)
SET(GLFW_BUILD_TESTS OFF)
SET(GLFW_INSTALL OFF)
SET(GLFW_USE_HYBRID_HPG OFF)
SET(GLFW_VULKAN_STATIC OFF)
add_subdirectory(third_party/glfw)

option(USE_ASSIMP OFF)
if (USE_ASSIMP)
	message("==============================================")
	message("                     Assimp                   ")
	message("==============================================")
	SET(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
	SET(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON)
	SET(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
	SET(ASSIMP_BUILD_SAMPLES OFF)
	SET(ASSIMP_BUILD_TESTS OFF)
	SET(ASSIMP_INSTALL OFF)
	SET(ASSIMP_INSTALL_PDB OFF)
	SET(ASSIMP_NO_EXPORT ON)
	SET(BUILD_DOCS OFF)
	SET(BUILD_FRAMEWORK OFF)
	add_subdirectory(third_party/assimp)
endif (USE_ASSIMP)

message("==============================================")
message("                 ShaderWriter                 ")
message("==============================================")
set(SDW_BUILD_EXPORTER_GLSL OFF)
set(SDW_BUILD_EXPORTER_HLSL OFF)
set(SDW_BUILD_EXPORTER_SPIRV ON)
set(SDW_BUILD_TESTS OFF)
set(SDW_BUILD_SDW OFF)
set(SDW_GENERATE_SOURCE OFF)
set(PROJECTS_PACKAGE_WIX OFF)
set(PROJECTS_USE_PRETTY_PRINTING OFF)
add_subdirectory(third_party/ShaderWriter)

option(USE_SHADERC OFF)
if (USE_SHADERC)
	message("==============================================")
	message("                   shaderc                    ")
	message("==============================================")
	set(gtest_force_shared_crt OFF)
	set(SHADERC_ENABLE_NV_EXTENSIONS OFF)
	set(SHADERC_SKIP_INSTALL ON)
	set(SHADERC_SKIP_TESTS ON)
	add_subdirectory("$ENV{VULKAN_SDK}/shaderc" "${CMAKE_CURRENT_BINARY_DIR}-shaderc")
endif (USE_SHADERC)

option(USE_IMGUI OFF)
if (USE_IMGUI)
	message("==============================================")
	message("                     ImGUI                    ")
	message("==============================================")
	set(IMGUI_DIR CACHE PATH "")
	include_directories(${IMGUI_DIR} ${IMGUI_DIR}/examples)
	set(IMGUI_FILES ${IMGUI_DIR}/imconfig.h ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp ${IMGUI_DIR}/examples/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
endif (USE_IMGUI)

message("==============================================")
message("                  VkRenderer                  ")
message("==============================================")
add_library(VkRenderer STATIC
	src/VkRenderer/Buffer.cpp
	src/VkRenderer/Buffer.hpp
	src/VkRenderer/CommandBuffer.cpp
	src/VkRenderer/CommandBuffer.hpp
	src/VkRenderer/CommandBuffer.inl
	src/VkRenderer/Framebuffer.cpp
	src/VkRenderer/Framebuffer.hpp
	src/VkRenderer/Image.cpp
	src/VkRenderer/Image.hpp
	src/VkRenderer/ImageView.cpp
	src/VkRenderer/ImageView.hpp
	src/VkRenderer/Mandelbulb.cpp
	src/VkRenderer/Mandelbulb.hpp
	src/VkRenderer/Material.cpp
	src/VkRenderer/Material.hpp
	src/VkRenderer/Renderer.cpp
	src/VkRenderer/Renderer.hpp
	src/VkRenderer/RenderPass.cpp
	src/VkRenderer/RenderPass.hpp
	src/VkRenderer/RenderWindow.cpp
	src/VkRenderer/RenderWindow.hpp
	src/VkRenderer/Texture.cpp
	src/VkRenderer/Texture.hpp
	src/VkRenderer/VulkanDevice.cpp
	src/VkRenderer/VulkanDevice.hpp
	src/VkRenderer/VulkanDevice.inl

	${IMGUI_FILES}
)

set_target_properties(VkRenderer PROPERTIES FOLDER "")

target_include_directories(VkRenderer
	PUBLIC
		src/VkRenderer

	PRIVATE
		"${Vulkan_INCLUDE_DIRS}"
		third_party/include
		third_party/glfw/include
		third_party/ShaderWriter/include
)

target_link_libraries(VkRenderer PRIVATE glfw sdwShaderWriter sdwCompilerSpirV)

if (USE_SHADERC)
	target_link_libraries(VkRenderer PUBLIC shaderc)
	target_compile_definitions(VkRenderer PUBLIC -DUSE_SHADERC)
endif (USE_SHADERC)

if (USE_ASSIMP)
	target_include_directories(VkRenderer
		PRIVATE
			third_party/assimp/include
	)
	target_link_libraries(VkRenderer PRIVATE assimp)
endif (USE_ASSIMP)

set_target_properties(VkRenderer PROPERTIES CXX_STANDARD 17)

if(MSVC)
	target_compile_options(VkRenderer PRIVATE "/MP")
endif()

message("==============================================")
message("                   MainTest                   ")
message("==============================================")
add_executable(MainTest test/main_test/main.cpp)
target_include_directories(MainTest
	PRIVATE
		include/VkRenderer
		"${Vulkan_INCLUDE_DIRS}"
		third_party/include
		third_party/ShaderWriter/include
)

target_link_libraries(MainTest PRIVATE VkRenderer sdwShaderWriter sdwCompilerSpirV)
set_target_properties(MainTest PROPERTIES CXX_STANDARD 17)
set_target_properties(MainTest PROPERTIES FOLDER "")

if(MSVC)
	target_compile_options(MainTest PRIVATE "/MP")
endif()

#message(ok)

unset(PROJECTS_BINARIES_OUTPUT_DIR)

add_compile_definitions(_AMD64_)
