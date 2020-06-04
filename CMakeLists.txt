cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY FOLDER "Third Party")

project(VkRenderer)

unset(PROJECTS_BINARIES_OUTPUT_DIR)

# set(GLFW3_PATH "" CACHE PATH "Path to GLFW3 cmake config file (glfw3Config.cmake or glfw3-config.cmake)")
# set(ASSIMP_PATH "" CACHE PATH "Path to ASSIMP cmake config file (assimpConfig.cmake or assimp-config.cmake)")

list(APPEND CMAKE_MODULE_PATH ${GLFW3_PATH})
list(APPEND CMAKE_MODULE_PATH ${ASSIMP_PATH})

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Assimp REQUIRED)
find_package(ShaderWriter REQUIRED)
# find_package(shaderc REQUIRED)

# message("==============================================")
# message("                     GLFW                     ")
# message("==============================================")
# SET(BUILD_SHARED_LIBS OFF)
# SET(GLFW_BUILD_DOCS OFF)
# SET(GLFW_BUILD_EXAMPLES OFF)
# SET(GLFW_BUILD_TESTS OFF)
# SET(GLFW_INSTALL OFF)
# SET(GLFW_USE_HYBRID_HPG OFF)
# SET(GLFW_VULKAN_STATIC OFF)
# add_subdirectory(third_party/glfw)

# option(USE_ASSIMP OFF)
# if (USE_ASSIMP)
# 	message("==============================================")
# 	message("                     Assimp                   ")
# 	message("==============================================")
# 	SET(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
# 	SET(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON)
# 	SET(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
# 	SET(ASSIMP_BUILD_SAMPLES OFF)
# 	SET(ASSIMP_BUILD_TESTS OFF)
# 	SET(ASSIMP_INSTALL OFF)
# 	SET(ASSIMP_INSTALL_PDB OFF)
# 	SET(ASSIMP_NO_EXPORT ON)
# 	SET(BUILD_DOCS OFF)
# 	SET(BUILD_FRAMEWORK OFF)
# 	add_subdirectory(third_party/assimp)
# endif (USE_ASSIMP)

# message("==============================================")
# message("                 ShaderWriter                 ")
# message("==============================================")
# set(SDW_BUILD_EXPORTER_GLSL OFF)
# set(SDW_BUILD_EXPORTER_HLSL OFF)
# set(SDW_BUILD_EXPORTER_SPIRV ON)
# set(SDW_BUILD_TESTS OFF)
# set(SDW_BUILD_SDW OFF)
# set(SDW_GENERATE_SOURCE OFF)
# set(PROJECTS_PACKAGE_WIX OFF)
# set(PROJECTS_USE_PRETTY_PRINTING OFF)
# add_subdirectory(third_party/ShaderWriter)

# message("==============================================")
# message("                   shaderc                    ")
# message("==============================================")
# set(gtest_force_shared_crt OFF)
# set(SHADERC_ENABLE_NV_EXTENSIONS OFF)
# set(SHADERC_SKIP_INSTALL ON)
# set(SHADERC_SKIP_TESTS ON)
# add_subdirectory(third_party/shaderc)

set(SHADERC_LIBRARY_DIR "" CACHE PATH "Path to shaderc library directory (folder containing shaderc_combined.lib)")
set(SHADERC_INCLUDE_DIR "" CACHE PATH "Path to shaderc include directory (folder containing shaderc/shaderc.h)")

message("==============================================")
message("                     ImGUI                    ")
message("==============================================")
set(IMGUI_DIR CACHE PATH "")
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/examples)
set(IMGUI_FILES ${IMGUI_DIR}/imconfig.h ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp ${IMGUI_DIR}/examples/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_widgets.cpp)

message("==============================================")
message("                  VkRenderer                  ")
message("==============================================")
set(MATERIAL_FILES
	src/VkRenderer/Materials/CustomMaterial.cpp
	src/VkRenderer/Materials/CustomMaterial.hpp
	src/VkRenderer/Materials/DefaultMaterial.cpp
	src/VkRenderer/Materials/DefaultMaterial.hpp
)

set(SOURCE_FILES
	src/VkRenderer/BrdfLut.cpp
	src/VkRenderer/BrdfLut.hpp
	src/VkRenderer/BrdfLutGenerator.cpp
	src/VkRenderer/BrdfLutGenerator.hpp
	src/VkRenderer/Buffer.cpp
	src/VkRenderer/Buffer.hpp
	src/VkRenderer/CommandBuffer.cpp
	src/VkRenderer/CommandBuffer.hpp
	src/VkRenderer/CommandBuffer.inl
	src/VkRenderer/Cubemap.cpp
	src/VkRenderer/Cubemap.hpp
	src/VkRenderer/DepthTexture.cpp
	src/VkRenderer/DepthTexture.hpp
	src/VkRenderer/EquirectangularToCubemap.cpp
	src/VkRenderer/EquirectangularToCubemap.hpp
	src/VkRenderer/EquirectangularToIrradianceMap.cpp
	src/VkRenderer/EquirectangularToIrradianceMap.hpp
	src/VkRenderer/Framebuffer.cpp
	src/VkRenderer/Framebuffer.hpp
	src/VkRenderer/Image.cpp
	src/VkRenderer/Image.hpp
	src/VkRenderer/ImageView.cpp
	src/VkRenderer/ImageView.hpp
	src/VkRenderer/IrradianceMap.cpp
	src/VkRenderer/IrradianceMap.hpp
	src/VkRenderer/Mandelbulb.cpp
	src/VkRenderer/Mandelbulb.hpp
	src/VkRenderer/Material.cpp
	src/VkRenderer/Material.hpp
	src/VkRenderer/Model.cpp
	src/VkRenderer/Model.hpp
	src/VkRenderer/MyShaderWriter.hpp
	src/VkRenderer/PbrShadingModel.cpp
	src/VkRenderer/PbrShadingModel.hpp
	src/VkRenderer/PrefilterCubemap.cpp
	src/VkRenderer/PrefilterCubemap.hpp
	src/VkRenderer/PrefilteredCubemap.cpp
	src/VkRenderer/PrefilteredCubemap.hpp
	src/VkRenderer/Renderer.cpp
	src/VkRenderer/Renderer.hpp
	src/VkRenderer/RenderPass.cpp
	src/VkRenderer/RenderPass.hpp
	src/VkRenderer/RenderWindow.cpp
	src/VkRenderer/RenderWindow.hpp
	src/VkRenderer/Scene.cpp
	src/VkRenderer/Scene.hpp
	src/VkRenderer/SceneObject.cpp
	src/VkRenderer/SceneObject.hpp
	src/VkRenderer/ShaderBall.cpp
	src/VkRenderer/ShaderBall.hpp
	src/VkRenderer/Skybox.cpp
	src/VkRenderer/Skybox.hpp
	src/VkRenderer/StagingBuffer.cpp
	src/VkRenderer/StagingBuffer.hpp
	src/VkRenderer/StandardMesh.cpp
	src/VkRenderer/StandardMesh.hpp
	src/VkRenderer/Texture2D.cpp
	src/VkRenderer/Texture2D.hpp
	src/VkRenderer/TextureInterface.hpp
	src/VkRenderer/UniformBuffer.cpp
	src/VkRenderer/UniformBuffer.hpp
	src/VkRenderer/VulkanDevice.cpp
	src/VkRenderer/VulkanDevice.hpp
	src/VkRenderer/VulkanDevice.inl
	src/VkRenderer/VulkanHelperStructs.hpp
)

add_library(VkRenderer STATIC
	src/third_party/stb_image.cpp

	${SOURCE_FILES}
	${MATERIAL_FILES}
	${IMGUI_FILES}
)

set_target_properties(VkRenderer PROPERTIES FOLDER "")

source_group("Sources" FILES ${SOURCE_FILES})
source_group("Materials" FILES ${MATERIAL_FILES})
source_group("ImGui" FILES ${IMGUI_FILES})

target_include_directories(VkRenderer
	PUBLIC
		src/VkRenderer

		"${Vulkan_INCLUDE_DIRS}"
		third_party/include
		${GLFW3_INCLUDE_DIR}
		third_party/ShaderWriter/include
		${SHADERC_INCLUDE_DIR}
)

# target_link_libraries(VkRenderer PRIVATE ${GLFW3_LIBRARY} sdw::sdwShaderWriter sdw::sdwCompilerSpirV assimp::assimp shaderc)
link_directories(${SHADERC_LIBRARY_DIR})
target_link_libraries(VkRenderer PUBLIC glfw sdw::sdwShaderWriter sdw::sdwCompilerSpirV assimp::assimp shaderc_combined)

# if (USE_ASSIMP)
# 	target_include_directories(VkRenderer
# 		PRIVATE
# 			third_party/assimp/include
# 	)
# 	target_link_libraries(VkRenderer PRIVATE assimp)
# endif (USE_ASSIMP)

set_target_properties(VkRenderer PROPERTIES CXX_STANDARD 17)

if(MSVC)
	target_compile_options(VkRenderer PUBLIC "/MP")
endif()

message("==============================================")
message("                   MainTest                   ")
message("==============================================")
add_executable(MainTest test/main_test/main.cpp)
target_include_directories(MainTest
	PUBLIC
		include/VkRenderer
		"${Vulkan_INCLUDE_DIRS}"
		# third_party/include
		${GLFW3_INCLUDE_DIR}
		# third_party/ShaderWriter/include
)

target_link_libraries(MainTest PUBLIC ${GLFW3_LIBRARY} VkRenderer)
set_target_properties(MainTest PROPERTIES CXX_STANDARD 17)
set_target_properties(MainTest PROPERTIES FOLDER "")

if(MSVC)
	target_compile_options(MainTest PUBLIC "/MP")
endif()

#message(ok)

unset(PROJECTS_BINARIES_OUTPUT_DIR)

add_compile_definitions(_AMD64_)
