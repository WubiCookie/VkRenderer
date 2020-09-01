cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY FOLDER "Third Party")

project(VkRenderer)

unset(PROJECTS_BINARIES_OUTPUT_DIR)

list(APPEND CMAKE_MODULE_PATH ${GLFW3_PATH})
list(APPEND CMAKE_MODULE_PATH ${ASSIMP_PATH})

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Assimp REQUIRED)
find_package(ShaderWriter REQUIRED)

set(SHADERC_LIBRARY_DIR "" CACHE PATH "Path to shaderc library directory (folder containing shaderc_combined.lib)")
set(SHADERC_INCLUDE_DIR "" CACHE PATH "Path to shaderc include directory (folder containing shaderc/shaderc.h)")

message("==============================================")
message("                     ImGUI                    ")
message("==============================================")
set(IMGUI_DIR "third_party/imgui")
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/examples)
set(IMGUI_FILES
	${IMGUI_DIR}/imconfig.h
	${IMGUI_DIR}/imgui.h
	${IMGUI_DIR}/examples/imgui_impl_glfw.cpp
	src/VkRenderer/my_imgui_impl_vulkan.cpp
	src/VkRenderer/my_imgui_impl_vulkan.h
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
)

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
	src/VkRenderer/Texture3D.cpp
	src/VkRenderer/Texture3D.hpp
	src/VkRenderer/TextureInterface.hpp
	src/VkRenderer/UniformBuffer.cpp
	src/VkRenderer/UniformBuffer.hpp
	src/VkRenderer/VolumeScene.cpp
	src/VkRenderer/VolumeScene.hpp
	src/VkRenderer/VulkanDevice.cpp
	src/VkRenderer/VulkanDevice.hpp
	src/VkRenderer/VulkanDevice.inl
	src/VkRenderer/VulkanHelperStructs.hpp
)

set(NIFTI_FILES
	third_party/niftilib/nifti1.h
	third_party/niftilib/nifti1_io.h
	third_party/niftilib/nifti1_io.c
	third_party/niftilib/nifti1_io_version.h
)

set(ZNZLIB_FILES
	third_party/znzlib/znzlib_version.h
	third_party/znzlib/znzlib.h
	third_party/znzlib/znzlib.c
)

add_library(VkRenderer STATIC
	src/third_party/stb_image.cpp

	${SOURCE_FILES}
	${MATERIAL_FILES}
	${IMGUI_FILES}
	${NIFTI_FILES}
	${ZNZLIB_FILES}
)

set_target_properties(VkRenderer PROPERTIES FOLDER "")

source_group("Sources" FILES ${SOURCE_FILES})
source_group("Materials" FILES ${MATERIAL_FILES})
source_group("ImGui" FILES ${IMGUI_FILES})
source_group("Nifti" FILES ${NIFTI_FILES})
source_group("znzlib" FILES ${ZNZLIB_FILES})

target_include_directories(VkRenderer
	PUBLIC
		src/VkRenderer

		"${Vulkan_INCLUDE_DIRS}"
		third_party/include
		third_party/niftilib
		third_party/znzlib
		${GLFW3_INCLUDE_DIR}
		${SHADERC_INCLUDE_DIR}
)

link_directories(${SHADERC_LIBRARY_DIR})
target_link_libraries(VkRenderer PRIVATE glfw sdw::ShaderWriter sdw::CompilerSpirV assimp::assimp shaderc_combined)

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
		${GLFW3_INCLUDE_DIR}
)

target_link_libraries(MainTest PUBLIC VkRenderer glfw sdw::ShaderWriter sdw::CompilerSpirV)
set_target_properties(MainTest PROPERTIES CXX_STANDARD 17)
set_target_properties(MainTest PROPERTIES FOLDER "")

if(MSVC)
	target_compile_options(MainTest PUBLIC "/MP")
endif()

unset(PROJECTS_BINARIES_OUTPUT_DIR)

add_compile_definitions(_AMD64_)
