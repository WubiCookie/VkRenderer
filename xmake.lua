add_repositories("my-repo myrepo")

--add_packagedirs("packages")
--add_requires("shaderwriter")
add_requires("shaderwriter")
--add_requires("shaderwriter", {static = false})

--add_requires("glfw", {shared=false, build_tests=false})
--add_requires("imgui", {debug=true})
add_requires("assimp")

set_project("VkRenderer")

add_rules("mode.debug", "mode.release", "mode.releasedbg", "c++")

if is_plat("windows") then

	-- link libcmt.lib
	add_cxflags("-MT")

	-- no msvcrt.lib
	add_ldflags("-nodefaultlib:\"msvcrt.lib\"")
end

-- if (is_plat("windows")) then
-- 	add_cxflags(is_mode("debug") and "/MTd" or "/MT")
-- 	add_cxxflags(is_mode("debug") and "/MTd" or "/MT")
-- 	--add_cxxflags("/bigobj", "/ZI", "/Zc:__cplusplus", "/Zc:referenceBinding", "/Zc:throwingNew")
-- 	--add_cxxflags("/FC")
-- end

-- add_cxflags("/MP")
-- add_cxxflags("/MP")

target("imgui")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_packages("glfw")
	add_files(
		"third_party/imgui/examples/imgui_impl_glfw.cpp",
		"src/VkRenderer/my_imgui_impl_vulkan.cpp",
		"third_party/imgui/imgui.cpp",
		"third_party/imgui/imgui_draw.cpp",
		"third_party/imgui/imgui_demo.cpp",
		"third_party/imgui/imgui_widgets.cpp"
	)
	add_headerfiles(
		"third_party/imgui/imconfig.h",
		"third_party/imgui/imgui.h",
		"src/VkRenderer/my_imgui_impl_vulkan.h"
	)
	add_includedirs(
		"third_party/imgui",
		"third_party/include",
		"third_party/imgui/examples",
		"$(env VULKAN_SDK)/Include", { public = true })

--[[
target("sdwShaderAST")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_files("external/ShaderWriter/source/ShaderAST/**.cpp")
	add_headerfiles("external/ShaderWriter/include/ShaderAST/**.hpp", "external/ShaderWriter/include/ShaderAST/**.inl")
	add_includedirs("external/ShaderWriter/include/ShaderAST", { public = true })
	add_includedirs("external/ShaderWriter/include", { public = true })
	add_defines("ShaderAST_Static")

target("sdwShaderWriter")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_files("external/ShaderWriter/source/ShaderWriter/**.cpp")
	add_headerfiles("external/ShaderWriter/include/ShaderWriter/**.hpp", "external/ShaderWriter/include/ShaderWriter/**.inl")
	add_includedirs("external/ShaderWriter/include/ShaderWriter", { public = true })
	add_includedirs("external/ShaderWriter/include", { public = true })
	add_defines("ShaderWriter_Static")

target("sdwCompilerSpirV")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_files("external/ShaderWriter/source/CompilerSpirV/**.cpp")
	add_headerfiles("external/ShaderWriter/include/CompilerSpirV/**.hpp", "external/ShaderWriter/include/CompilerSpirV/**.inl")
	add_includedirs("external/ShaderWriter/include/CompilerSpirV", { public = true })
	add_includedirs("external/ShaderWriter/include", { public = true })
	add_defines("CompilerSpirV_Static")

target("shaderwriter")
	on_build(function (target)
		if is_mode("debug") then
			os.exec("cmake -B$(buildir)/buildShaderWriter -Sexternal/ShaderWriter -DPROJECTS_OUTPUT_DIR='$(buildir)/buildShaderWriter' -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF -DPROJECTS_USE_PRETTY_PRINTING=OFF -DPROJECTS_PACKAGE_WIX=OFF -DSDW_BUILD_EXPORTER_GLSL=OFF -DSDW_BUILD_EXPORTER_HLSL=OFF -DSDW_BUILD_EXPORTER_SPIRV_STATIC=ON -DSDW_BUILD_VULKAN_LAYER=OFF -DCMAKE_BUILD_TYPE='Debug' -DCMAKE_CXX_FLAGS_DEBUG='/MTd /Zi /Ob0 /Od /RTC1' -DCMAKE_C_FLAGS_DEBUG='/MTd /Zi /Ob0 /Od /RTC1'")
			os.exec("cmake --build $(buildir)/buildShaderWriter --config Debug")
		end
		if is_mode("release") then
			os.exec("cmake -B$(buildir)/buildShaderWriter -Sexternal/ShaderWriter -DPROJECTS_OUTPUT_DIR='$(buildir)/buildShaderWriter' -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF -DPROJECTS_USE_PRETTY_PRINTING=OFF -DPROJECTS_PACKAGE_WIX=OFF -DSDW_BUILD_EXPORTER_GLSL=OFF -DSDW_BUILD_EXPORTER_HLSL=OFF -DSDW_BUILD_EXPORTER_SPIRV_STATIC=ON -DSDW_BUILD_VULKAN_LAYER=OFF -DCMAKE_BUILD_TYPE='Release' -DCMAKE_CXX_FLAGS_RELEASE='/MT /O2 /Ob2 /DNDEBUG' -DCMAKE_C_FLAGS_RELEASE='/MT /O2 /Ob2 /DNDEBUG'")
			os.exec("cmake --build $(buildir)/buildShaderWriter --config Release")
		end
		if is_mode("release") then
			os.exec("cmake -B$(buildir)/buildShaderWriter -Sexternal/ShaderWriter -DPROJECTS_OUTPUT_DIR='$(buildir)/buildShaderWriter' -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF -DPROJECTS_USE_PRETTY_PRINTING=OFF -DPROJECTS_PACKAGE_WIX=OFF -DSDW_BUILD_EXPORTER_GLSL=OFF -DSDW_BUILD_EXPORTER_HLSL=OFF -DSDW_BUILD_EXPORTER_SPIRV_STATIC=ON -DSDW_BUILD_VULKAN_LAYER=OFF -DCMAKE_BUILD_TYPE='Release' -DCMAKE_CXX_FLAGS_RELEASE='/MT /O2 /Ob2 /DNDEBUG' -DCMAKE_C_FLAGS_RELEASE='/MT /O2 /Ob2 /DNDEBUG'")
			os.exec("cmake --build $(buildir)/buildShaderWriter --config RelWithDebInfo")
		end
	end)

	if is_mode("debug") then
		add_linkdirs("$(buildir)/buildShaderWriter/binaries/x64/Debug/lib/")
		add_links("sdwCompilerSpirVd", { public = true })
		add_links("sdwShaderASTd", { public = true })
		add_links("sdwShaderWriterd", { public = true })
	end
	if is_mode("release") then
		add_linkdirs("$(buildir)/buildShaderWriter/binaries/x64/Release/lib/")
		add_links("sdwCompilerSpirV", { public = true })
		add_links("sdwShaderAST", { public = true })
		add_links("sdwShaderWriter", { public = true })
	end
	if is_mode("release") then
		add_linkdirs("$(buildir)/buildShaderWriter/binaries/x64/RelWithDebInfo/lib/")
		add_links("sdwCompilerSpirV", { public = true })
		add_links("sdwShaderAST", { public = true })
		add_links("sdwShaderWriter", { public = true })
	end
--]]



option("useSTB")
	set_showmenu(true)
	set_description("Use STB as a backend for TextureLoaderFrontend")
option_end()

option("useTexas")
	set_showmenu(true)
	set_description("Use Texas as a backend for TextureLoaderFrontend")
option_end()

option("useASTC_CODEC")
	set_showmenu(true)
	set_description("Use astc-codec as a backend for TextureLoaderFrontend")
option_end()

option("useLoadDDS")
	set_showmenu(true)
	set_description("Use LoadDDS as a backend for TextureLoaderFrontend")
option_end()

target("TextureLoaderFrontend")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")

	if has_config("useSTB") then
		add_headerfiles("src/TextureLoaderFrontend/stb_image.h")
		add_defines("TEXTURELOADERFRONTEND_USE_STB")
	end

	if has_config("useTexas") then
		-- TODO
		add_defines("TEXTURELOADERFRONTEND_USE_TEXAS")
	end

	if has_config("useASTC_CODEC") then
		-- TODO
		add_defines("TEXTURELOADERFRONTEND_USE_ASTC_CODEC")
	end

	if has_config("useLoadDDS") then
		add_files("src/TextureLoaderFrontend/load_dds.cpp")
		add_headerfiles("src/TextureLoaderFrontend/load_dds.hpp")
		add_defines("TEXTURELOADERFRONTEND_USE_LOAD_DDS")
	end

	add_includedirs(
		"src/TextureLoaderFrontend", {public = true}
	)
	add_files("src/TextureLoaderFrontend/TextureLoaderFrontend.cpp")
	add_headerfiles("src/TextureLoaderFrontend/TextureLoaderFrontend.hpp")



target("VkRenderer")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_packages("glfw", "imgui", "assimp")
	add_packages("shaderwriter")
	--add_deps("sdwShaderAST", "sdwShaderWriter", "sdwCompilerSpirV", "TextureLoaderFrontend", "shaderwriter")
	add_deps("TextureLoaderFrontend", "imgui")
	add_includedirs(
		"third_party/include",
		"third_party/imgui/examples",
		"$(env VULKAN_SDK)/Include",
		"src/VkRenderer",
		"src/VkRenderer/Materials", {public = true}
	)
	--add_includedirs("external/ShaderWriter/include", { public = true })
	--add_includedirs("C:/Users/Charles/AppData/Local/.xmake/packages/s/shaderwriter/master/7001040a8533492597b595810d0adc4e/include")
	--add_linkdirs("$(buildir)/buildShaderWriter/binaries/x64/Release/lib/")
	--add_links("sdwCompilerSpirV")
	--add_links("sdwShaderAST")
	--add_links("sdwShaderWriter")
	add_headerfiles("src/third_party/**.h")
	add_files("src/third_party/**.cpp")
	--add_files("src/VkRenderer/**.cpp")
	--add_headerfiles("src/VkRenderer/**.h")
	--add_headerfiles("src/VkRenderer/**.hpp")

	add_files(
		"src/VkRenderer/Materials/CustomMaterial.cpp",
		"src/VkRenderer/Materials/DefaultMaterial.cpp",
		"src/VkRenderer/BrdfLut.cpp",
		"src/VkRenderer/BrdfLutGenerator.cpp",
		"src/VkRenderer/Buffer.cpp",
		"src/VkRenderer/CommandBuffer.cpp",
		"src/VkRenderer/CommandBufferPool.cpp",
		"src/VkRenderer/CommandPool.cpp",
		"src/VkRenderer/Cubemap.cpp",
		"src/VkRenderer/DepthTexture.cpp",
		"src/VkRenderer/EquirectangularToCubemap.cpp",
		"src/VkRenderer/EquirectangularToIrradianceMap.cpp",
		"src/VkRenderer/Framebuffer.cpp",
		"src/VkRenderer/Image.cpp",
		"src/VkRenderer/TextureFactory.cpp",
		"src/VkRenderer/ImageView.cpp",
		"src/VkRenderer/IrradianceMap.cpp",
		"src/VkRenderer/LightTransport.cpp",
		"src/VkRenderer/LightTransportRenderPasses.cpp",
		"src/VkRenderer/LightTransportShaderModules.cpp",
		"src/VkRenderer/LightTransportDescriptorsObjects.cpp",
		"src/VkRenderer/LightTransportPipelines.cpp",
		"src/VkRenderer/LightTransportBuffers.cpp",
		"src/VkRenderer/LightTransportImages.cpp",
		"src/VkRenderer/LightTransportFramebuffers.cpp",
		"src/VkRenderer/LightTransportUpdateDescriptorSets.cpp",
		"src/VkRenderer/Mandelbulb.cpp",
		"src/VkRenderer/Material.cpp",
		"src/VkRenderer/Model.cpp",
		"src/VkRenderer/MyShaderWriter.cpp",
		"src/VkRenderer/PbrShadingModel.cpp",
		"src/VkRenderer/PipelineFactory.cpp",
		"src/VkRenderer/PrefilterCubemap.cpp",
		"src/VkRenderer/PrefilteredCubemap.cpp",
		"src/VkRenderer/Renderer.cpp",
		"src/VkRenderer/RenderPass.cpp",
		"src/VkRenderer/RenderWindow.cpp",
		"src/VkRenderer/Scene.cpp",
		"src/VkRenderer/SceneObject.cpp",
		"src/VkRenderer/ShaderBall.cpp",
		"src/VkRenderer/Skybox.cpp",
		"src/VkRenderer/StagingBuffer.cpp",
		"src/VkRenderer/StandardMesh.cpp",
		"src/VkRenderer/Texture1D.cpp",
		"src/VkRenderer/Texture2D.cpp",
		"src/VkRenderer/UniformBuffer.cpp",
		"src/VkRenderer/VertexInputHelper.cpp",
		"src/VkRenderer/VulkanDevice.cpp"
	)
	add_headerfiles(
		"src/VkRenderer/Materials/CustomMaterial.hpp",
		"src/VkRenderer/Materials/DefaultMaterial.hpp",
		"src/VkRenderer/BrdfLut.hpp",
		"src/VkRenderer/BrdfLutGenerator.hpp",
		"src/VkRenderer/Buffer.hpp",
		"src/VkRenderer/CommandBuffer.hpp",
		"src/VkRenderer/CommandBuffer.inl",
		"src/VkRenderer/CommandBufferPool.hpp",
		"src/VkRenderer/CommandPool.hpp",
		"src/VkRenderer/Cubemap.hpp",
		"src/VkRenderer/DepthTexture.hpp",
		"src/VkRenderer/EquirectangularToCubemap.hpp",
		"src/VkRenderer/EquirectangularToIrradianceMap.hpp",
		"src/VkRenderer/Framebuffer.hpp",
		"src/VkRenderer/Image.hpp",
		"src/VkRenderer/TextureFactory.hpp",
		"src/VkRenderer/ImageView.hpp",
		"src/VkRenderer/IrradianceMap.hpp",
		"src/VkRenderer/LightTransport.hpp",
		"src/VkRenderer/Mandelbulb.hpp",
		"src/VkRenderer/Material.hpp",
		"src/VkRenderer/Model.hpp",
		"src/VkRenderer/MyShaderWriter.hpp",
		"src/VkRenderer/MyShaderWriter.inl",
		"src/VkRenderer/PbrShadingModel.hpp",
		"src/VkRenderer/PipelineFactory.hpp",
		"src/VkRenderer/PrefilterCubemap.hpp",
		"src/VkRenderer/PrefilteredCubemap.hpp",
		"src/VkRenderer/Renderer.hpp",
		"src/VkRenderer/RenderPass.hpp",
		"src/VkRenderer/RenderWindow.hpp",
		"src/VkRenderer/Scene.hpp",
		"src/VkRenderer/SceneObject.hpp",
		"src/VkRenderer/ShaderBall.hpp",
		"src/VkRenderer/Skybox.hpp",
		"src/VkRenderer/StagingBuffer.hpp",
		"src/VkRenderer/StandardMesh.hpp",
		"src/VkRenderer/Texture1D.hpp",
		"src/VkRenderer/Texture2D.hpp",
		"src/VkRenderer/TextureInterface.hpp",
		"src/VkRenderer/UniformBuffer.hpp",
		"src/VkRenderer/VertexInputHelper.hpp",
		"src/VkRenderer/VulkanDevice.hpp",
		"src/VkRenderer/VulkanDevice.inl",
		"src/VkRenderer/VulkanHelperStructs.hpp"
	)
	--if is_mode("debug") then
	--	add_linkdirs("$(buildir)/buildShaderWriter/binaries/x64/Debug/lib/")
	--	add_links("sdwCompilerSpirVd")
	--	add_links("sdwShaderASTd")
	--	add_links("sdwShaderWriterd")
	--end
	--if is_mode("release") then

	--end
	--add_defines("ShaderAST_Static", "ShaderWriter_Static", "CompilerSpirV_Static")



target("MainTest")
	set_kind("binary")
	set_languages("cxx17")
	add_deps("VkRenderer")
	add_packages("shaderwriter")
	add_files("test/main_test/main.cpp")
