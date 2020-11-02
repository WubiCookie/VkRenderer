#pragma once

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"
#include "VertexInputHelper.hpp"
#include "VulkanDevice.hpp"
#include "cdm_vulkan.hpp"

#include <unordered_map>

#define LocaleW(writer, name, value)                                          \
	auto name = writer.declLocale(#name, value);
#define FLOATW(writer, name) auto name = writer.declLocale<Float>(#name);
#define VEC2W(writer, name) auto name = writer.declLocale<Vec2>(#name);
#define VEC3W(writer, name) auto name = writer.declLocale<Vec3>(#name);
#define ConstantW(writer, name, value)                                        \
	auto name = writer.declConstant(#name, value);

#define Locale(name, value) LocaleW(writer, name, value)
#define FLOAT(name) FLOATW(writer, name)
#define VEC2(name) VEC2W(writer, name)
#define VEC3(name) VEC3W(writer, name)
#define Constant(name, value) ConstantW(writer, name, value)

namespace cdm
{
// void vertexFunction(Vec3& inOutPosition, Vec3& inOutNormal);
using MaterialVertexFunction =
    sdw::Function<sdw::Float, sdw::InOutVec3, sdw::InOutVec3>;

// void fragmentFunction(UInt inMaterialInstanceIndex,
//                       Vec4& inOutAlbedo,
//                       Vec2& inOutUv,
//                       Vec3& inOutWsPosition,
//                       Vec3& inOutWsNormal,
//                       Vec3& inOutWsTangent,
//                       Float& inOutMetalness,
//                       Float& inOutRoughness);
using MaterialFragmentFunction =
    sdw::Function<sdw::Float, sdw::InUInt, sdw::InOutVec4, sdw::InOutVec2,
                  sdw::InOutVec3, sdw::InOutVec3, sdw::InOutVec3,
                  sdw::InOutFloat, sdw::InOutFloat>;

// Vec4 combinedMaterialShading(UInt inMaterialInstanceIndex,
//                              Vec3 wsPosition,
//                              Vec2 uv,
//                              Vec3 wsNormal,
//                              Vec3 wsTangent,
//                              Vec3 wsViewPosition);
using CombinedMaterialShadingFragmentFunction =
    sdw::Function<sdw::Vec4, sdw::InUInt, sdw::InVec3, sdw::InVec2,
                  sdw::InVec3, sdw::InVec3, sdw::InVec3>;

struct VertexShaderHelperResult
{
	VertexInputHelper vertexInputHelper;
	std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>> descriptors;
	UniqueShaderModule module;
};

struct FragmentShaderHelperResult
{
	std::unordered_map<uint32_t, VkFormat> outputAttachments;
	std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>> descriptors;
	UniqueShaderModule module;
};

struct ComputeShaderHelperResult
{
	std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>> descriptors;
	UniqueShaderModule module;
};

class VertexWriter;
class FragmentWriter;
class ComputeWriter;

class Ubo : public sdw::Ubo
{
public:
	Ubo(VertexWriter& writer, std::string const& name, uint32_t bind,
	    uint32_t set,
	    ast::type::MemoryLayout layout = ast::type::MemoryLayout::eStd140);
	Ubo(FragmentWriter& writer, std::string const& name, uint32_t bind,
	    uint32_t set,
	    ast::type::MemoryLayout layout = ast::type::MemoryLayout::eStd140);
	Ubo(ComputeWriter& writer, std::string const& name, uint32_t bind,
	    uint32_t set,
	    ast::type::MemoryLayout layout = ast::type::MemoryLayout::eStd140);

	Ubo(const Ubo&) = default;
	Ubo(Ubo&&) = default;
	Ubo& operator=(const Ubo&) = default;
	Ubo& operator=(Ubo&&) = default;
};

class VertexWriter : public sdw::VertexWriter
{
	friend Ubo;

	VertexInputHelper m_vertexInputHelper;
	std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>>
	    m_descriptors;

public:
	UniqueShaderModule createShaderModule(const VulkanDevice& vk) const;
	VertexShaderHelperResult createHelperResult(const VulkanDevice& vk) const;

	inline const std::vector<
	    std::pair<uint32_t, VkDescriptorSetLayoutBinding>>&
	getDescriptors() const;

	inline void addInputBinding(
	    const VkVertexInputBindingDescription& binding);
	inline void addInputBinding(
	    uint32_t binding, uint32_t stride,
	    VkVertexInputRate rate =
	        VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX);
	inline void addInputAttribute(
	    const VkVertexInputAttributeDescription& attribute);
	inline void addInputAttribute(uint32_t location, uint32_t binding,
	                              VkFormat format, uint32_t offset);
	inline void addInputAttribute(uint32_t location, uint32_t binding,
	                              VkFormat format);

	inline const VertexInputHelper& getVertexInputHelper() const;

#pragma region Sampled Image declaration
	/**
	 *name
	 *	Sampled Image declaration.
	 */
	/**@{*/
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>
	declSampledImage(std::string const& name, uint32_t binding, uint32_t set);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
	declSampledImage(std::string const& name, uint32_t binding, uint32_t set,
	                 bool enabled);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
	declSampledImageArray(std::string const& name, uint32_t binding,
	                      uint32_t set, uint32_t dimension);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>>
	declSampledImageArray(std::string const& name, uint32_t binding,
	                      uint32_t set, uint32_t dimension, bool enabled);
	/**@}*/
#pragma endregion
#pragma region Image declaration
	/**
	 *name
	 *	Image declaration.
	 */
	/**@{*/
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>
	declImage(std::string const& name, uint32_t binding, uint32_t set);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
	declImage(std::string const& name, uint32_t binding, uint32_t set,
	          bool enabled);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Array<
	    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
	declImageArray(std::string const& name, uint32_t binding, uint32_t set,
	               uint32_t dimension);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>>
	declImageArray(std::string const& name, uint32_t binding, uint32_t set,
	               uint32_t dimension, bool enabled);
	/**@}*/
#pragma endregion
#pragma region Input declaration
	/**
	 *name
	 *	Input declaration.
	 */
	/**@{*/
	template <typename T>
	inline T declInput(std::string const& name, uint32_t location);
	template <typename T>
	inline T declInput(std::string const& name, uint32_t location,
	                   uint32_t attributes);
	template <typename T>
	inline sdw::Array<T> declInputArray(std::string const& name,
	                                    uint32_t location, uint32_t dimension);
	template <typename T>
	inline sdw::Array<T> declInputArray(std::string const& name,
	                                    uint32_t location, uint32_t dimension,
	                                    uint32_t attributes);
	template <typename T>
	inline sdw::Optional<T> declInput(std::string const& name,
	                                  uint32_t location, bool enabled);
	template <typename T>
	inline sdw::Optional<T> declInput(std::string const& name,
	                                  uint32_t location, uint32_t attributes,
	                                  bool enabled);
	template <typename T>
	inline sdw::Optional<sdw::Array<T>> declInputArray(std::string const& name,
	                                                   uint32_t location,
	                                                   uint32_t dimension,
	                                                   bool enabled);
	template <typename T>
	inline sdw::Optional<sdw::Array<T>> declInputArray(std::string const& name,
	                                                   uint32_t location,
	                                                   uint32_t dimension,
	                                                   uint32_t attributes,
	                                                   bool enabled);
	template <typename T>
	inline T declInput(std::string const& name, uint32_t location,
	                   bool enabled, T const& defaultValue);
	template <typename T>
	inline T declInput(std::string const& name, uint32_t location,
	                   uint32_t attributes, bool enabled,
	                   T const& defaultValue);
	template <typename T>
	inline sdw::Array<T> declInputArray(std::string const& name,
	                                    uint32_t location, uint32_t dimension,
	                                    uint32_t attributes, bool enabled,
	                                    std::vector<T> const& defaultValue);
	template <typename T>
	inline sdw::Array<T> declInputArray(std::string const& name,
	                                    uint32_t location, uint32_t dimension,
	                                    bool enabled,
	                                    std::vector<T> const& defaultValue);
	/**@}*/
#pragma endregion
};

class FragmentWriter : public sdw::FragmentWriter
{
	friend Ubo;

	std::unordered_map<uint32_t, VkFormat> m_outputAttachments;
	std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>>
	    m_descriptors;

public:
	UniqueShaderModule createShaderModule(const VulkanDevice& vk) const;
	FragmentShaderHelperResult createHelperResult(
	    const VulkanDevice& vk) const;

	inline const std::vector<
	    std::pair<uint32_t, VkDescriptorSetLayoutBinding>>&
	getDescriptors() const;

	inline const std::unordered_map<uint32_t, VkFormat>& getOutputAttachments()
	    const;

#pragma region Sampled Image declaration
	/**
	 *name
	 *	Sampled Image declaration.
	 */
	/**@{*/
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>
	declSampledImage(std::string const& name, uint32_t binding, uint32_t set);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
	declSampledImage(std::string const& name, uint32_t binding, uint32_t set,
	                 bool enabled);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
	declSampledImageArray(std::string const& name, uint32_t binding,
	                      uint32_t set, uint32_t dimension);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>>
	declSampledImageArray(std::string const& name, uint32_t binding,
	                      uint32_t set, uint32_t dimension, bool enabled);
	/**@}*/
#pragma endregion
#pragma region Image declaration
	/**
	 *name
	 *	Image declaration.
	 */
	/**@{*/
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>
	declImage(std::string const& name, uint32_t binding, uint32_t set);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
	declImage(std::string const& name, uint32_t binding, uint32_t set,
	          bool enabled);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Array<
	    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
	declImageArray(std::string const& name, uint32_t binding, uint32_t set,
	               uint32_t dimension);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>>
	declImageArray(std::string const& name, uint32_t binding, uint32_t set,
	               uint32_t dimension, bool enabled);
	/**@}*/
#pragma endregion
#pragma region Output declaration
	/**
	 *name
	 *	Output declaration.
	 */
	/**@{*/
	template <typename T>
	inline T declOutput(std::string const& name, uint32_t location);
	template <typename T>
	inline T declOutput(std::string const& name, uint32_t location,
	                    uint32_t attributes);
	template <typename T>
	inline sdw::Array<T> declOutputArray(std::string const& name,
	                                     uint32_t location,
	                                     uint32_t dimension);
	template <typename T>
	inline sdw::Array<T> declOutputArray(std::string const& name,
	                                     uint32_t location, uint32_t dimension,
	                                     uint32_t attributes);
	template <typename T>
	inline sdw::Optional<T> declOutput(std::string const& name,
	                                   uint32_t location, bool enabled);
	template <typename T>
	inline sdw::Optional<T> declOutput(std::string const& name,
	                                   uint32_t location, uint32_t attributes,
	                                   bool enabled);
	template <typename T>
	inline sdw::Optional<sdw::Array<T>> declOutputArray(
	    std::string const& name, uint32_t location, uint32_t dimension,
	    bool enabled);
	template <typename T>
	inline sdw::Optional<sdw::Array<T>> declOutputArray(
	    std::string const& name, uint32_t location, uint32_t dimension,
	    uint32_t attributes, bool enabled);
	/**@}*/
#pragma endregion
};

class ComputeWriter : public sdw::ComputeWriter
{
	friend Ubo;

	std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>>
	    m_descriptors;

public:
	UniqueShaderModule createShaderModule(const VulkanDevice& vk) const;
	ComputeShaderHelperResult createHelperResult(const VulkanDevice& vk) const;

	void addDescriptor(uint32_t binding, uint32_t set, VkDescriptorType type);

	inline const std::vector<
	    std::pair<uint32_t, VkDescriptorSetLayoutBinding>>&
	getDescriptors() const;

#pragma region Sampled Image declaration
	/**
	 *name
	 *	Sampled Image declaration.
	 */
	/**@{*/
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>
	declSampledImage(std::string const& name, uint32_t binding, uint32_t set);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
	declSampledImage(std::string const& name, uint32_t binding, uint32_t set,
	                 bool enabled);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
	declSampledImageArray(std::string const& name, uint32_t binding,
	                      uint32_t set, uint32_t dimension);
	template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
	          bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>>
	declSampledImageArray(std::string const& name, uint32_t binding,
	                      uint32_t set, uint32_t dimension, bool enabled);
	/**@}*/
#pragma endregion
#pragma region Image declaration
	/**
	 *name
	 *	Image declaration.
	 */
	/**@{*/
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>
	declImage(std::string const& name, uint32_t binding, uint32_t set);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
	declImage(std::string const& name, uint32_t binding, uint32_t set,
	          bool enabled);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Array<
	    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
	declImageArray(std::string const& name, uint32_t binding, uint32_t set,
	               uint32_t dimension);
	template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
	          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
	inline sdw::Optional<
	    sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>>
	declImageArray(std::string const& name, uint32_t binding, uint32_t set,
	               uint32_t dimension, bool enabled);
	/**@}*/
#pragma endregion
};
}  // namespace cdm

#include "MyShaderWriter.inl"
