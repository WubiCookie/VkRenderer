-- if is_arch("x64") then

set_arch("x64")

if is_plat("windows") then
	-- if is_mode("release") then
	--     add_cxflags("-MT")
	-- elseif is_mode("debug") then
	--     add_cxflags("-MTd")
	-- end
	-- add_syslinks("ws2_32")
	add_ldflags("-nodefaultlib:msvcrt.lib")
end

package("shaderwriter1")

	set_homepage("https://github.com/DragonJoker/ShaderWriter")
	set_description("Library used to write shaders from C++, and export them in either GLSL, HLSL or SPIR-V.")

	set_urls("https://github.com/DragonJoker/ShaderWriter.git")
	add_versions("1.0.1", "780090957d163aa60e161e4b17acff09fbcf7d03")

	add_deps("cmake")

	add_links("sdwShaderWriter", "sdwCompilerHlsl", "sdwCompilerGlsl", "sdwCompilerSpirV", "sdwShaderAST")

	on_install("windows", "macosx", "linux", function (package)
		local configs =
		{
			"-DSDW_BUILD_TESTS=OFF",
			"-DSDW_BUILD_EXPORTERS=ON",
			"-DSDW_BUILD_STATIC_SDW=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_BUILD_EXPORTER_GLSL_STATIC=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_BUILD_EXPORTER_HLSL_STATIC=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_BUILD_EXPORTER_SPIRV_STATIC=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_GENERATE_SOURCE=OFF",
			"-DPROJECTS_USE_PRECOMPILED_HEADERS=OFF",
			"-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"),
			"-DCMAKE_CXX_FLAGS='/DWIN32 /D_WINDOWS /W3 /GR /EHsc /MP'",
			"-DCMAKE_CXX_FLAGS_DEBUG='/MDd /Zi /Ob0 /Od /RTC1'",
			"-DCMAKE_CXX_FLAGS_MINSIZEREL='/MD /O1 /Ob1 /DNDEBUG'",
			"-DCMAKE_CXX_FLAGS_RELEASE='/MD /O2 /Ob2 /DNDEBUG'",
			"-DCMAKE_CXX_FLAGS_RELWITHDEBINFO='/MD /Zi /O2 /Ob1 /DNDEBUG'",
			"-DCMAKE_C_FLAGS='/DWIN32 /D_WINDOWS /W3 /MP'",
			"-DCMAKE_C_FLAGS_DEBUG='/MDd /Zi /Ob0 /Od /RTC1'",
			"-DCMAKE_C_FLAGS_MINSIZEREL='/MD /O1 /Ob1 /DNDEBUG'",
			"-DCMAKE_C_FLAGS_RELEASE='/MD /O2 /Ob2 /DNDEBUG'",
			"-DCMAKE_C_FLAGS_RELWITHDEBINFO='/MD /Zi /O2 /Ob1 /DNDEBUG'"
		}
		import("package.tools.cmake").install(package, configs)
	end)

	-- on_test(function (package)
	--  assert(package:check_cxxsnippets({test = [[
	--      // #include <CompilerGlsl/compileGlsl.hpp>
	--      // #include <CompilerSpirV/compileSpirV.hpp>
	--      // #include <ShaderWriter/Intrinsics/Intrinsics.hpp>
	--      // #include <ShaderWriter/Source.hpp>
	--      static void test()
	--      {
	--          sdw::VertexWriter writer;
	--      }
	--  ]]}, {configs = {languages = "c++17"}, includes = {"CompilerGlsl/compileGlsl.hpp", "CompilerSpirV/compileSpirV.hpp", "ShaderWriter/Intrinsics/Intrinsics.hpp", "ShaderWriter/Source.hpp"}}))
	-- end)
package_end()
--]]


-- add_requires("shaderwriter1",  {debug = is_mode("debug")})
add_requires("shaderwriter1")

--[[package("tinyobjloader")

	set_homepage("https://github.com/tinyobjloader/tinyobjloader")
	set_description("Tiny but powerful single file wavefront obj loader")
	set_license("MIT")

	add_urls("https://github.com/tinyobjloader/tinyobjloader/archive/v$(version).tar.gz",
			 "https://github.com/tinyobjloader/tinyobjloader.git")
	add_versions("1.0.7", "b9d08b675ba54b9cb00ffc99eaba7616d0f7e6f6b8947a7e118474e97d942129")

	add_configs("double", {description = "Use double precision floating numbers.", default = false, type = "boolean"})

	on_install("macosx", "linux", "windows", "mingw", "android", "iphoneos", function (package)
		local kind = package:config("shared") and "shared" or "static"
		io.writefile("xmake.lua", string.format([[
			add_rules("mode.debug", "mode.release")
			target("tinyobjloader")
				set_kind("%s")
				%s
				add_files("tiny_obj_loader.cc")
				add_headerfiles("tiny_obj_loader.h")
		] ], kind, (package:config("double") and "add_defines(\"TINYOBJLOADER_USE_DOUBLE\")" or "")))
		import("package.tools.xmake").install(package)
	end)

	on_test(function (package)
		assert(package:check_cxxsnippets({test = [[
			#include <vector>
			void test() {
				tinyobj::attrib_t attrib;
				std::vector<tinyobj::shape_t> shapes;
				std::vector<tinyobj::material_t> materials;
			}
		] ]}, {configs = {languages = "c++11"}, includes = "tiny_obj_loader.h"}))
	end)
package_end()
--]]


-- add_requires("assimp",        {debug = is_mode("debug"), config = { shared = false, no_export = true, build_tests = false }})
add_requires("assimp",        {config = { shared = false, no_export = true, build_tests = false }})
-- add_requires("tinyobjloader", {debug = is_mode("debug")})
add_requires("glfw",          {debug = is_mode("debug")})

set_project("VkRenderer")

-- add_rules("mode.debug", "mode.release", "mode.releasedbg", "c++")
add_rules("mode.debug", "mode.release", "mode.releasedbg")
-- add_rules("mode.debug", "mode.release")
-- add_rules("mode.release")

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
target_end()


--[[
target("sdwShaderAST")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_files("external/ShaderWriter/source/ShaderAST/**.cpp")
	add_headerfiles("external/ShaderWriter/include/ShaderAST/**.hpp", "external/ShaderWriter/include/ShaderAST/**.inl")
	add_includedirs("external/ShaderWriter/include/ShaderAST", { public = true })
	add_includedirs("external/ShaderWriter/include", { public = true })
	add_defines("ShaderAST_Static", { public = true })
target_end()

target("sdwShaderWriter")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_files("external/ShaderWriter/source/ShaderWriter/**.cpp")
	add_headerfiles("external/ShaderWriter/include/ShaderWriter/**.hpp", "external/ShaderWriter/include/ShaderWriter/**.inl")
	add_includedirs("external/ShaderWriter/include/ShaderWriter", { public = true })
	add_includedirs("external/ShaderWriter/include", { public = true })
	add_defines("ShaderWriter_Static", { public = true })
target_end()

target("sdwCompilerSpirV")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	add_files("external/ShaderWriter/source/CompilerSpirV/**.cpp")
	add_headerfiles("external/ShaderWriter/include/CompilerSpirV/**.hpp", "external/ShaderWriter/include/CompilerSpirV/**.inl")
	add_includedirs("external/ShaderWriter/include/CompilerSpirV", { public = true })
	add_includedirs("external/ShaderWriter/include", { public = true })
	add_defines("CompilerSpirV_Static", { public = true })
target_end()
--]]

--[[
target("shaderwriter")
	on_build(function (target)
		if is_mode("debug") then
			os.exec("cmake -B$(buildir)/../buildShaderWriter -Sexternal/ShaderWriter -DPROJECTS_OUTPUT_DIR='$(buildir)/../buildShaderWriter' -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF -DPROJECTS_USE_PRETTY_PRINTING=OFF -DPROJECTS_PACKAGE_WIX=OFF -DSDW_BUILD_EXPORTER_GLSL=OFF -DSDW_BUILD_EXPORTER_HLSL=OFF -DSDW_BUILD_EXPORTER_SPIRV_STATIC=ON -DSDW_BUILD_VULKAN_LAYER=OFF -DCMAKE_BUILD_TYPE='Debug' -DCMAKE_CXX_FLAGS_DEBUG='/i /Ob0 /Od /RTC1' -DCMAKE_C_FLAGS_DEBUG='/Zi /Ob0 /Od /RTC1'")
			os.exec("cmake --build $(buildir)/../buildShaderWriter --config Debug")
		end
		if is_mode("release") then
			os.exec("cmake -B$(buildir)/../buildShaderWriter -Sexternal/ShaderWriter -DPROJECTS_OUTPUT_DIR='$(buildir)/../buildShaderWriter' -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF -DPROJECTS_USE_PRETTY_PRINTING=OFF -DPROJECTS_PACKAGE_WIX=OFF -DSDW_BUILD_EXPORTER_GLSL=OFF -DSDW_BUILD_EXPORTER_HLSL=OFF -DSDW_BUILD_EXPORTER_SPIRV_STATIC=ON -DSDW_BUILD_VULKAN_LAYER=OFF -DCMAKE_BUILD_TYPE='Release' -DCMAKE_CXX_FLAGS_RELEASE='/O2 /Ob2 /DNDEBUG' -DCMAKE_C_FLAGS_RELEASE='/O2 /Ob2 /DNDEBUG'")
			os.exec("cmake --build $(buildir)/../buildShaderWriter --config Release")
		end
		if is_mode("releasedbg") then
			os.exec("cmake -B$(buildir)/../buildShaderWriter -Sexternal/ShaderWriter -DPROJECTS_OUTPUT_DIR='$(buildir)/../buildShaderWriter' -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF -DPROJECTS_USE_PRETTY_PRINTING=OFF -DPROJECTS_PACKAGE_WIX=OFF -DSDW_BUILD_EXPORTER_GLSL=OFF -DSDW_BUILD_EXPORTER_HLSL=OFF -DSDW_BUILD_EXPORTER_SPIRV_STATIC=ON -DSDW_BUILD_VULKAN_LAYER=OFF -DCMAKE_BUILD_TYPE='RelWithDebInfo' -DCMAKE_CXX_FLAGS_RELWITHDEBINFO='/O2 /Ob2 /DNDEBUG' -DCMAKE_C_FLAGS_RELWITHDEBINFO='/O2 /Ob2 /DNDEBUG'")
			os.exec("cmake --build $(buildir)/../buildShaderWriter --config RelWithDebInfo")
		end
	end)

	if is_mode("debug") then
		add_linkdirs("$(buildir)/../buildShaderWriter/binaries/x64/Debug/lib/")
		add_links("sdwCompilerSpirVd", { public = true })
		add_links("sdwShaderASTd", { public = true })
		add_links("sdwShaderWriterd", { public = true })
	end
	if is_mode("release") then
		add_linkdirs("$(buildir)/../buildShaderWriter/binaries/x64/Release/lib/")
		add_links("sdwCompilerSpirV", { public = true })
		add_links("sdwShaderAST", { public = true })
		add_links("sdwShaderWriter", { public = true })
	end
	if is_mode("releasedbg") then
		add_linkdirs("$(buildir)/../buildShaderWriter/binaries/x64/RelWithDebInfo/lib/")
		add_links("sdwCompilerSpirV", { public = true })
		add_links("sdwShaderAST", { public = true })
		add_links("sdwShaderWriter", { public = true })
	end
--]]

--[[
package("shaderwriter")

	set_homepage("https://github.com/DragonJoker/ShaderWriter")
	set_description("Library used to write shaders from C++, and export them in either GLSL, HLSL or SPIR-V.")

	set_urls("https://github.com/DragonJoker/ShaderWriter.git")
	add_versions("1.0", "a5ef99ff141693ef28cee0e464500888cabc65ad")

	add_deps("cmake")

	add_links("sdwShaderWriter", "sdwCompilerHlsl", "sdwCompilerGlsl", "sdwCompilerSpirV", "sdwShaderAST")

	on_install("windows", "macosx", "linux", function (package)
		local configs =
		{
			"-DSDW_BUILD_TESTS=OFF",
			"-DSDW_BUILD_EXPORTERS=ON",
			"-DSDW_BUILD_STATIC_SDW=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_BUILD_EXPORTER_GLSL_STATIC=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_BUILD_EXPORTER_HLSL_STATIC=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_BUILD_EXPORTER_SPIRV_STATIC=".. (package:config("shared") and "OFF" or "ON"),
			"-DSDW_GENERATE_SOURCE=OFF",
			"-DPROJECTS_USE_PRECOMPILED_HEADERS=OFF",
			"-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release")
		}
		import("package.tools.cmake").install(package, configs)
	end)
--]]



-- [[
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
target_end()
--]]

add_defines("CDM_MATHS_VECTORIZED")

target("VkRenderer")
	set_default(false)
	set_kind("static")
	set_languages("cxx17")
	-- add_packages("glfw")
	-- add_deps("shaderwriter")
	-- add_deps("sdwShaderAST", "sdwShaderWriter", "sdwCompilerSpirV")
	add_packages("glfw", "shaderwriter1")
	-- add_deps("TextureLoaderFrontend", "imgui")
	add_deps("imgui")

	add_defines("CompilerSpirV_Static")
	add_defines("ShaderWriter_Static")
	add_defines("ShaderAST_Static")

	add_includedirs(
		"third_party/include",
		"third_party/imgui/examples",
		"$(env VULKAN_SDK)/Include",
		"src/VkRenderer",
		"src/VkRenderer/Materials", {public = true}
	)

	add_headerfiles("src/third_party/**.h")
	add_files("src/third_party/**.cpp")

	add_files(
		"src/VkRenderer/BrdfLut.cpp",
		"src/VkRenderer/BrdfLutGenerator.cpp",
		"src/VkRenderer/Buffer.cpp",
		"src/VkRenderer/Camera.cpp",
		"src/VkRenderer/CommandBuffer.cpp",
		"src/VkRenderer/CommandBufferPool.cpp",
		"src/VkRenderer/CommandPool.cpp",
		"src/VkRenderer/Cubemap.cpp",
		"src/VkRenderer/DebugBox.cpp",
		"src/VkRenderer/DepthTexture.cpp",
		"src/VkRenderer/EquirectangularToCubemap.cpp",
		"src/VkRenderer/EquirectangularToIrradianceMap.cpp",
		"src/VkRenderer/Framebuffer.cpp",
		"src/VkRenderer/Image.cpp",
		"src/VkRenderer/ImageView.cpp",
		"src/VkRenderer/IrradianceMap.cpp",
		"src/VkRenderer/Material.cpp",
		"src/VkRenderer/Materials/CustomMaterial.cpp",
		"src/VkRenderer/Materials/DefaultMaterial.cpp",
		"src/VkRenderer/Model.cpp",
		"src/VkRenderer/MyShaderWriter.cpp",
		"src/VkRenderer/PbrShadingModel.cpp",
		"src/VkRenderer/PipelineFactory.cpp",
		"src/VkRenderer/PrefilterCubemap.cpp",
		"src/VkRenderer/PrefilteredCubemap.cpp",
		"src/VkRenderer/RenderApplication.cpp",
		"src/VkRenderer/Renderer.cpp",
		"src/VkRenderer/RenderPass.cpp",
		"src/VkRenderer/RenderWindow.cpp",
		"src/VkRenderer/Scene.cpp",
		"src/VkRenderer/SceneObject.cpp",
		"src/VkRenderer/Skybox.cpp",
		"src/VkRenderer/StagingBuffer.cpp",
		"src/VkRenderer/StandardMesh.cpp",
		"src/VkRenderer/Texture1D.cpp",
		"src/VkRenderer/Texture2D.cpp",
		"src/VkRenderer/TextureFactory.cpp",
		"src/VkRenderer/UniformBuffer.cpp",
		"src/VkRenderer/VertexInputHelper.cpp",
		"src/VkRenderer/VulkanDevice.cpp"
	)
	add_headerfiles(
		"src/VkRenderer/BrdfLut.hpp",
		"src/VkRenderer/BrdfLutGenerator.hpp",
		"src/VkRenderer/Buffer.hpp",
		"src/VkRenderer/Camera.hpp",
		"src/VkRenderer/CommandBuffer.hpp",
		"src/VkRenderer/CommandBuffer.inl",
		"src/VkRenderer/CommandBufferPool.hpp",
		"src/VkRenderer/CommandPool.hpp",
		"src/VkRenderer/Cubemap.hpp",
		"src/VkRenderer/DebugBox.inl",
		"src/VkRenderer/DepthTexture.hpp",
		"src/VkRenderer/EquirectangularToCubemap.hpp",
		"src/VkRenderer/EquirectangularToIrradianceMap.hpp",
		"src/VkRenderer/Framebuffer.hpp",
		"src/VkRenderer/Image.hpp",
		"src/VkRenderer/ImageView.hpp",
		"src/VkRenderer/IrradianceMap.hpp",
		"src/VkRenderer/Material.hpp",
		"src/VkRenderer/Materials/CustomMaterial.hpp",
		"src/VkRenderer/Materials/DefaultMaterial.hpp",
		"src/VkRenderer/Model.hpp",
		"src/VkRenderer/MyShaderWriter.hpp",
		"src/VkRenderer/MyShaderWriter.inl",
		"src/VkRenderer/PbrShadingModel.hpp",
		"src/VkRenderer/PipelineFactory.hpp",
		"src/VkRenderer/PrefilterCubemap.hpp",
		"src/VkRenderer/PrefilteredCubemap.hpp",
		"src/VkRenderer/RenderApplication.hpp",
		"src/VkRenderer/Renderer.hpp",
		"src/VkRenderer/RenderPass.hpp",
		"src/VkRenderer/RenderWindow.hpp",
		"src/VkRenderer/Scene.hpp",
		"src/VkRenderer/SceneObject.hpp",
		"src/VkRenderer/Skybox.hpp",
		"src/VkRenderer/StagingBuffer.hpp",
		"src/VkRenderer/StandardMesh.hpp",
		"src/VkRenderer/Texture1D.hpp",
		"src/VkRenderer/Texture2D.hpp",
		"src/VkRenderer/TextureFactory.hpp",
		"src/VkRenderer/TextureInterface.hpp",
		"src/VkRenderer/UniformBuffer.hpp",
		"src/VkRenderer/VertexInputHelper.hpp",
		"src/VkRenderer/VulkanDevice.hpp",
		"src/VkRenderer/VulkanDevice.inl",
		"src/VkRenderer/VulkanHelperStructs.hpp"
	)

	-- add_includedirs("external/ShaderWriter/include", { public = true })
	-- add_includedirs("C:/Users/Charles/AppData/Local/.xmake/packages/s/shaderwriter/master/7001040a8533492597b595810d0adc4e/include")
	-- add_linkdirs("$(buildir)/../buildShaderWriter/binaries/x64/Release/lib/")


	-- add_defines("CompilerSpirV_Static", "ShaderWriter_Static", "ShaderAST_Static")
	-- add_links("CompilerSpirV", "ShaderWriter", "ShaderAST")
target_end()







target("SpatialPartitionning")
	set_targetdir("bin")
	set_kind("binary")
	set_languages("cxx17")
	add_deps("VkRenderer")
	add_packages("assimp")
	add_packages("shaderwriter1")
	add_defines("CompilerSpirV_Static")
	add_defines("ShaderWriter_Static")
	add_defines("ShaderAST_Static")
	add_deps("imgui")
	add_files("test/SpatialPartitionning/*.cpp")
	add_headerfiles("test/SpatialPartitionning/*.hpp")
	add_includedirs("third_party/include")
target_end()

target("LightTransport")
	set_kind("binary")
	set_languages("cxx17")
	add_deps("VkRenderer")
	add_packages("imgui")
	add_files("test/LightTransport/*.cpp")
	add_headerfiles("test/LightTransport/*.hpp")
target_end()

target("Mandelbulb")
	set_kind("binary")
	set_languages("cxx17")
	add_deps("VkRenderer")
	add_packages("imgui")
	add_files("test/Mandelbulb/*.cpp")
	add_headerfiles("test/Mandelbulb/*.hpp")

	if is_mode("debug") then
		set_symbols("debug")
		set_optimize("none")
	end
target_end()

target("ShaderBall")
	set_kind("binary")
	set_languages("cxx17")
	add_deps("VkRenderer")
	add_packages("imgui", "assimp")
	add_files("test/ShaderBall/*.cpp")
	add_headerfiles("test/ShaderBall/*.hpp")
target_end()

-- end
