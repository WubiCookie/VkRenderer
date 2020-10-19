cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VkRenderer)

find_package(Vulkan REQUIRED)

set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_INSTALL OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
set(SDW_BUILD_EXPORTERS ON CACHE BOOL "")
set(SDW_BUILD_EXPORTER_GLSL OFF CACHE BOOL "")
set(SDW_BUILD_EXPORTER_GLSL_STATIC OFF CACHE BOOL "")
set(SDW_BUILD_EXPORTER_HLSL OFF CACHE BOOL "")
set(SDW_BUILD_EXPORTER_HLSL_STATIC OFF CACHE BOOL "")
set(SDW_BUILD_EXPORTER_SPIRV ON CACHE BOOL "")
set(SDW_BUILD_EXPORTER_SPIRV_STATIC ON CACHE BOOL "")
set(SDW_BUILD_STATIC_SDW ON CACHE BOOL "")
set(SDW_BUILD_TESTS OFF CACHE BOOL "")
set(SDW_BUILD_VULKAN_LAYER OFF CACHE BOOL "")
set(SDW_GENERATE_SOURCE ON CACHE BOOL "")

message("==============================================")
message("                     GLFW                     ")
message("==============================================")
set(GLFW_DIR "external/glfw")
add_subdirectory(${GLFW_DIR})

message("==============================================")
message("                    Assimp                    ")
message("==============================================")
set(ASSIMP_DIR "external/assimp")
add_subdirectory(${ASSIMP_DIR})

message("==============================================")
message("                 ShaderWriter                 ")
message("==============================================")
set(SHADERWRITER_DIR "external/ShaderWriter")
add_subdirectory(${SHADERWRITER_DIR})

# message("==============================================")
# message("                   ShaderC                    ")
# message("==============================================")
# set(SHADERC_DIR "external/shaderc")
# add_subdirectory(${SHADERC_DIR})

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

set_target_properties(glfw PROPERTIES FOLDER "Third Party/GLFW3")

# set_target_properties(GenericCodeGen PROPERTIES FOLDER "Third Party/glslang")
# set_target_properties(glslang PROPERTIES FOLDER "Third Party/glslang")
# set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER "Third Party/glslang")
# set_target_properties(MachineIndependent PROPERTIES FOLDER "Third Party/glslang")
# set_target_properties(OGLCompiler PROPERTIES FOLDER "Third Party/glslang")
# set_target_properties(OSDependent PROPERTIES FOLDER "Third Party/glslang")
# set_target_properties(SPIRV PROPERTIES FOLDER "Third Party/glslang")

# set_target_properties(spirv-tools-build-version PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spirv-tools-header-DebugInfo PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spirv-tools-header-OpenCLDebugInfo100 PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-cldi100 PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-clspvreflection PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-debuginfo PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-spv-amd-gs PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-spv-amd-sb PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-spv-amd-sevp PROPERTIES FOLDER "Third Party/SPIRV-Tools build")
# set_target_properties(spv-tools-spv-amd-stm PROPERTIES FOLDER "Third Party/SPIRV-Tools build")

# set_target_properties(SPIRV-Tools-link PROPERTIES FOLDER "Third Party/SPIRV-Tools libraries")
# set_target_properties(SPIRV-Tools-opt PROPERTIES FOLDER "Third Party/SPIRV-Tools libraries")
# set_target_properties(SPIRV-Tools-reduce PROPERTIES FOLDER "Third Party/SPIRV-Tools libraries")
# set_target_properties(SPIRV-Tools-shared PROPERTIES FOLDER "Third Party/SPIRV-Tools libraries")
# set_target_properties(SPIRV-Tools-static PROPERTIES FOLDER "Third Party/SPIRV-Tools libraries")

# set_target_properties(spirv-tools-vimsyntax PROPERTIES FOLDER "Third Party/SPIRV-Tools utilities")

# set_target_properties(glslangValidator PROPERTIES FOLDER "Third Party/tools")

# set_target_properties(add-copyright PROPERTIES FOLDER "Third Party")
#set_target_properties(assimp PROPERTIES FOLDER "Third Party" COMPILE_OPTIONS "/WX- /MP /bigobj")
#get_target_property(ASSIMP_COMPILE_OPTIONS assimp COMPILE_OPTIONS)
#list(REMOVE_ITEM ASSIMP_COMPILE_OPTIONS "/WX")
#set_target_properties(assimp PROPERTIES FOLDER "Third Party" COMPILE_OPTIONS "${ASSIMP_COMPILE_OPTIONS}")
set_target_properties(assimp PROPERTIES FOLDER "Third Party")
if(MSVC)
    target_compile_options(assimp PUBLIC /WX-)
endif()
#message("${ASSIMP_COMPILE_OPTIONS}")
#message("${ASSIMP_COMPILE_OPTIONS}")
##set_target_properties(assimp PROPERTIES FOLDER "Third Party" COMPILE_OPTIONS "/WX-;${ASSIMP_COMPILE_OPTIONS};/WX-;")
#get_target_property(ASSIMP_COMPILE_OPTIONS assimp COMPILE_OPTIONS)
#message("${ASSIMP_COMPILE_OPTIONS}")
#target_compile_options(assimp PUBLIC "/WX-")
#target_compile_options(assimp BEFORE PUBLIC /WX-)
# set_target_properties(build-version PROPERTIES FOLDER "Third Party")
# set_target_properties(check-copyright PROPERTIES FOLDER "Third Party")
# set_target_properties(core_tables PROPERTIES FOLDER "Third Party")
# set_target_properties(enum_string_mapping PROPERTIES FOLDER "Third Party")
# set_target_properties(extinst_tables PROPERTIES FOLDER "Third Party")
# set_target_properties(glslang-build-info PROPERTIES FOLDER "Third Party")
# set_target_properties(glslc PROPERTIES FOLDER "Third Party")
# set_target_properties(glslc_exe PROPERTIES FOLDER "Third Party")
# set_target_properties(install-headers PROPERTIES FOLDER "Third Party")
set_target_properties(IrrXML PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc_combined_genfile PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc_combined-pkg-config PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc_shared PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc_static-pkg-config PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc_util PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc-online-compile PROPERTIES FOLDER "Third Party")
# set_target_properties(shaderc-pkg-config PROPERTIES FOLDER "Third Party")
# set_target_properties(spirv-tools-pkg-config PROPERTIES FOLDER "Third Party")
# set_target_properties(spirv-tools-shared-pkg-config PROPERTIES FOLDER "Third Party")
# set_target_properties(testdata PROPERTIES FOLDER "Third Party")
set_target_properties(uninstall PROPERTIES FOLDER "Third Party")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Third Party")
set_target_properties(zlibstatic PROPERTIES FOLDER "Third Party")

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
    src/VkRenderer/CommandPool.cpp
    src/VkRenderer/CommandPool.hpp
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
    src/VkRenderer/ImageFactory.cpp
    src/VkRenderer/ImageFactory.hpp
    src/VkRenderer/ImageView.cpp
    src/VkRenderer/ImageView.hpp
    src/VkRenderer/IrradianceMap.cpp
    src/VkRenderer/IrradianceMap.hpp
    src/VkRenderer/LightTransport.cpp
    src/VkRenderer/LightTransport.hpp
	src/VkRenderer/LightTransportRenderPasses.cpp
	src/VkRenderer/LightTransportShaderModules.cpp
	src/VkRenderer/LightTransportDescriptorsObjects.cpp
	src/VkRenderer/LightTransportPipelines.cpp
	src/VkRenderer/LightTransportBuffers.cpp
	src/VkRenderer/LightTransportImages.cpp
	src/VkRenderer/LightTransportFramebuffers.cpp
	src/VkRenderer/LightTransportUpdateDescriptorSets.cpp
    src/VkRenderer/Mandelbulb.cpp
    src/VkRenderer/Mandelbulb.hpp
    src/VkRenderer/Material.cpp
    src/VkRenderer/Material.hpp
    src/VkRenderer/Model.cpp
    src/VkRenderer/Model.hpp
    src/VkRenderer/MyShaderWriter.cpp
    src/VkRenderer/MyShaderWriter.hpp
    src/VkRenderer/MyShaderWriter.inl
    src/VkRenderer/PbrShadingModel.cpp
    src/VkRenderer/PbrShadingModel.hpp
    src/VkRenderer/PipelineFactory.cpp
    src/VkRenderer/PipelineFactory.hpp
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
    src/VkRenderer/Texture1D.cpp
    src/VkRenderer/Texture1D.hpp
    src/VkRenderer/Texture2D.cpp
    src/VkRenderer/Texture2D.hpp
    src/VkRenderer/TextureInterface.hpp
    src/VkRenderer/UniformBuffer.cpp
    src/VkRenderer/UniformBuffer.hpp
    src/VkRenderer/VertexInputHelper.cpp
    src/VkRenderer/VertexInputHelper.hpp
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
        # ${SHADERC_INCLUDE_DIR}
)

# link_directories(${SHADERC_LIBRARY_DIR})
#target_link_libraries(VkRenderer PRIVATE glfw sdw::ShaderWriter sdw::CompilerSpirV assimp::assimp shaderc_combined)
target_link_libraries(VkRenderer PRIVATE glfw sdw::ShaderWriter sdw::CompilerSpirV assimp::assimp )

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
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/bin")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/bin")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/bin")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_NAME "MainTest.exe")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_NAME_DEBUG "MainTestd.exe")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_NAME_RELEASE "MainTest.exe")
set_target_properties(MainTest PROPERTIES RUNTIME_OUTPUT_NAME_RELWITHDEBINFO "MainTest_RelWithDebInfo.exe")

if(MSVC)
    target_compile_options(MainTest PUBLIC "/MP")
endif()

add_compile_definitions(_AMD64_)

#file(DOWNLOAD sftp://c.seizilles@sirius.isart.lan:10022/JeuVideo/P_CLASSE/MGP_2021/4_Partage_Classe/bunny-skyboxHDR.7z "${CMAKE_CURRENT_LIST_DIR}/resources/bunny-skyboxHDR.tar")
#execute_process(
#    COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_CURRENT_LIST_DIR}/resources/bunny-skyboxHDR.tar"
#    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/resources/"
#)
