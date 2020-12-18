#pragma once

#include "MyShaderWriter.hpp"

#include <type_traits>

namespace cdm
{
inline const std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>>&
VertexWriter::getDescriptors() const
{
	return m_descriptors;
}

inline void VertexWriter::addInputBinding(
	const VkVertexInputBindingDescription& binding)
{
	m_vertexInputHelper.addInputBinding(binding);
}

inline void VertexWriter::addInputBinding(uint32_t binding, uint32_t stride,
										  VkVertexInputRate rate)
{
	m_vertexInputHelper.addInputBinding(binding, stride, rate);
}

inline void VertexWriter::addInputAttribute(
	const VkVertexInputAttributeDescription& attribute)
{
	m_vertexInputHelper.addInputAttribute(attribute);
}

inline void VertexWriter::addInputAttribute(uint32_t location,
											uint32_t binding, VkFormat format,
											uint32_t offset)
{
	m_vertexInputHelper.addInputAttribute(location, binding, format, offset);
}

inline void VertexWriter::addInputAttribute(uint32_t location,
											uint32_t binding, VkFormat format)
{
	m_vertexInputHelper.addInputAttribute(location, binding, format);
}

// inline vk::PipelineVertexInputStateCreateInfo
// VertexWriter::getVertexInputStateCreateInfo()
//{
//	return m_vertexInputHelper.getCreateInfo();
//}

inline const VertexInputHelper& VertexWriter::getVertexInputHelper() const
{
	return m_vertexInputHelper;
}

#pragma region Sampled Image declaration
/**
 *name
 *   Sampled Image declaration.
 */
/**@{*/
template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>
VertexWriter::declSampledImage(std::string const& name, uint32_t binding,
							   uint32_t set)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::VertexWriter::declSampledImage(name, binding, set);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
VertexWriter::declSampledImage(std::string const& name, uint32_t binding,
							   uint32_t set, bool enabled)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::VertexWriter::declSampledImage(name, binding, set, enabled);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
VertexWriter::declSampledImageArray(std::string const& name, uint32_t binding,
									uint32_t set, uint32_t dimension)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = dimension;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::VertexWriter::declSampledImageArray(name, binding, set,
													dimension);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
	sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>>
VertexWriter::declSampledImageArray(std::string const& name, uint32_t binding,
									uint32_t set, uint32_t dimension,
									bool enabled)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = dimension;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::VertexWriter::declSampledImageArray(name, binding, set,
													dimension, enabled);
}
/**@}*/
#pragma endregion
#pragma region Image declaration
/**
 *name
 *   Image declaration.
 */
/**@{*/
template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>
VertexWriter::declImage(std::string const& name, uint32_t binding,
						uint32_t set)
{
	return sdw::VertexWriter::declImage(name, binding, set);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
	sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
VertexWriter::declImage(std::string const& name, uint32_t binding,
						uint32_t set, bool enabled)
{
	return sdw::VertexWriter::declImage(name, binding, set, enabled);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
VertexWriter::declImageArray(std::string const& name, uint32_t binding,
							 uint32_t set, uint32_t dimension)
{
	return sdw::VertexWriter::declImageArray(name, binding, set, dimension);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
	sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>>
VertexWriter::declImageArray(std::string const& name, uint32_t binding,
							 uint32_t set, uint32_t dimension, bool enabled)
{
	return sdw::VertexWriter::declImageArray(name, binding, set, dimension,
											 enabled);
}
/**@}*/
#pragma endregion
#pragma region Input declaration
/**
 *name
 *   Input declaration.
 */
/**@{*/
template <typename T>
inline T VertexWriter::declInput(std::string const& name, uint32_t location)
{
	addInputBinding(0, 0);

	if constexpr (std::is_same_v<T, sdw::Float>)
		addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
	else
		abort();

	return sdw::VertexWriter::declInput<T>(name, location);
}

template <typename T>
inline T VertexWriter::declInput(std::string const& name, uint32_t location,
								 uint32_t attributes)
{
	addInputBinding(0, 0);

	if constexpr (std::is_same_v<T, sdw::Float>)
		addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
	else
		abort();

	return sdw::VertexWriter::declInput<T>(name, location, attributes);
}

template <typename T>
inline sdw::Array<T> VertexWriter::declInputArray(std::string const& name,
												  uint32_t location,
												  uint32_t dimension)
{
	addInputBinding(0, 0);

	if constexpr (std::is_same_v<T, sdw::Float>)
		addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
	else
		abort();

	return sdw::VertexWriter::declInputArray<T>(name, location, dimension);
}

template <typename T>
inline sdw::Array<T> VertexWriter::declInputArray(std::string const& name,
												  uint32_t location,
												  uint32_t dimension,
												  uint32_t attributes)
{
	addInputBinding(0, 0);

	if constexpr (std::is_same_v<T, sdw::Float>)
		addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
	else
		abort();

	return sdw::VertexWriter::declInputArray<T>(name, location, dimension,
												attributes);
}

template <typename T>
inline sdw::Optional<T> VertexWriter::declInput(std::string const& name,
												uint32_t location,
												bool enabled)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInput<T>(name, location, enabled);
}

template <typename T>
inline sdw::Optional<T> VertexWriter::declInput(std::string const& name,
												uint32_t location,
												uint32_t attributes,
												bool enabled)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInput<T>(name, location, attributes,
										   enabled);
}

template <typename T>
inline sdw::Optional<sdw::Array<T>> VertexWriter::declInputArray(
	std::string const& name, uint32_t location, uint32_t dimension,
	bool enabled)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInputArray<T>(name, location, dimension,
												enabled);
}

template <typename T>
inline sdw::Optional<sdw::Array<T>> VertexWriter::declInputArray(
	std::string const& name, uint32_t location, uint32_t dimension,
	uint32_t attributes, bool enabled)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInputArray<T>(name, location, dimension,
												attributes, enabled);
}

template <typename T>
inline T VertexWriter::declInput(std::string const& name, uint32_t location,
								 bool enabled, T const& defaultValue)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInput<T>(name, location, dimension,
										   defaultValue);
}

template <typename T>
inline T VertexWriter::declInput(std::string const& name, uint32_t location,
								 uint32_t attributes, bool enabled,
								 T const& defaultValue)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInput<T>(name, location, attributes, enabled,
										   defaultValue);
}

template <typename T>
inline sdw::Array<T> VertexWriter::declInputArray(
	std::string const& name, uint32_t location, uint32_t dimension,
	uint32_t attributes, bool enabled, std::vector<T> const& defaultValue)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInputArray<T>(
		name, location, dimension, attributes, enabled, defaultValue);
}

template <typename T>
inline sdw::Array<T> VertexWriter::declInputArray(
	std::string const& name, uint32_t location, uint32_t dimension,
	bool enabled, std::vector<T> const& defaultValue)
{
	if (enabled)
	{
		addInputBinding(0, 0);

		if constexpr (std::is_same_v<T, sdw::Float>)
			addInputAttribute(location, 0, VK_FORMAT_R32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32_SFLOAT);
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			addInputAttribute(location, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
		else
			abort();
	}

	return sdw::VertexWriter::declInputArray<T>(name, location, dimension,
												enabled, defaultValue);
}
/**@}*/
#pragma endregion

inline const std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>>&
FragmentWriter::getDescriptors() const
{
	return m_descriptors;
}

inline const std::unordered_map<uint32_t, VkFormat>&
FragmentWriter::getOutputAttachments() const
{
	return m_outputAttachments;
}

#pragma region Sampled Image declaration
/**
 *name
 *  Sampled Image declaration.
 */
/**@{*/
template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>
FragmentWriter::declSampledImage(std::string const& name, uint32_t binding,
								 uint32_t set)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::FragmentWriter::declSampledImage<FormatT, DimT, ArrayedT,
												 DepthT, MsT>(name, binding,
															  set);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
FragmentWriter::declSampledImage(std::string const& name, uint32_t binding,
								 uint32_t set, bool enabled)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::FragmentWriter::declSampledImage<FormatT, DimT, ArrayedT,
												 DepthT, MsT>(name, binding,
															  set, enabled);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
FragmentWriter::declSampledImageArray(std::string const& name,
									  uint32_t binding, uint32_t set,
									  uint32_t dimension)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = dimension;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::FragmentWriter::declSampledImageArray<FormatT, DimT, ArrayedT,
													  DepthT, MsT>(
		name, binding, set, dimension);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
		  bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
	sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>>
FragmentWriter::declSampledImageArray(std::string const& name,
									  uint32_t binding, uint32_t set,
									  uint32_t dimension, bool enabled)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = dimension;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::FragmentWriter::declSampledImageArray<FormatT, DimT, ArrayedT,
													  DepthT, MsT>(
		name, binding, set, dimension, enabled);
}
/**@}*/
#pragma endregion
#pragma region Image declaration
/**
 *name
 *  Image declaration.
 */
/**@{*/
template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>
FragmentWriter::declImage(std::string const& name, uint32_t binding,
						  uint32_t set)
{
	return sdw::FragmentWriter::declImage<FormatT, AccessT, DimT, ArrayedT,
										  DepthT, MsT>(name, binding, set);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
	sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
FragmentWriter::declImage(std::string const& name, uint32_t binding,
						  uint32_t set, bool enabled)
{
	return sdw::FragmentWriter::declImage<FormatT, AccessT, DimT, ArrayedT,
										  DepthT, MsT>(name, binding, set,
													   enabled);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
FragmentWriter::declImageArray(std::string const& name, uint32_t binding,
							   uint32_t set, uint32_t dimension)
{
	return sdw::FragmentWriter::declImageArray<FormatT, AccessT, DimT,
											   ArrayedT, DepthT, MsT>(
		name, binding, set, dimension);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
		  ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
	sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>>
FragmentWriter::declImageArray(std::string const& name, uint32_t binding,
							   uint32_t set, uint32_t dimension, bool enabled)
{
	return sdw::FragmentWriter::declImageArray<FormatT, AccessT, DimT,
											   ArrayedT, DepthT, MsT>(
		name, binding, set, dimension, enabled);
}
/**@}*/
#pragma endregion
#pragma region Output declaration
/**
 *name
 *  Output declaration.
 */
/**@{*/
template <typename T>
inline T FragmentWriter::declOutput(std::string const& name, uint32_t location)
{
	if constexpr (std::is_same_v<T, sdw::Float>)
		m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::UInt>)
		m_outputAttachments[location] = VK_FORMAT_R32_UINT;
	// else if constexpr (std::is_same_v<T, sdw::Int>)
	// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
	else
		abort();

	return sdw::FragmentWriter::declOutput<T>(name, location);
}

template <typename T>
inline T FragmentWriter::declOutput(std::string const& name, uint32_t location,
									uint32_t attributes)
{
	if constexpr (std::is_same_v<T, sdw::Float>)
		m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::UInt>)
		m_outputAttachments[location] = VK_FORMAT_R32_UINT;
	// else if constexpr (std::is_same_v<T, sdw::Int>)
	// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
	else
		abort();

	return sdw::FragmentWriter::declOutput<T>(name, location, attributes);
}

template <typename T>
inline sdw::Array<T> FragmentWriter::declOutputArray(std::string const& name,
													 uint32_t location,
													 uint32_t dimension)
{
	if constexpr (std::is_same_v<T, sdw::Float>)
		m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::UInt>)
		m_outputAttachments[location] = VK_FORMAT_R32_UINT;
	// else if constexpr (std::is_same_v<T, sdw::Int>)
	// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
	else
		abort();

	return sdw::FragmentWriter::declOutputArray<T>(name, location, dimension);
}

template <typename T>
inline sdw::Array<T> FragmentWriter::declOutputArray(std::string const& name,
													 uint32_t location,
													 uint32_t dimension,
													 uint32_t attributes)
{
	if constexpr (std::is_same_v<T, sdw::Float>)
		m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec2>)
		m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec3>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::Vec4>)
		m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
	else if constexpr (std::is_same_v<T, sdw::UInt>)
		m_outputAttachments[location] = VK_FORMAT_R32_UINT;
	// else if constexpr (std::is_same_v<T, sdw::Int>)
	// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
	else
		abort();

	return sdw::FragmentWriter::declOutputArray<T>(name, location, dimension,
												   attributes);
}

template <typename T>
inline sdw::Optional<T> FragmentWriter::declOutput(std::string const& name,
												   uint32_t location,
												   bool enabled)
{
	if (enabled)
	{
		if constexpr (std::is_same_v<T, sdw::Float>)
			m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::UInt>)
			m_outputAttachments[location] = VK_FORMAT_R32_UINT;
		// else if constexpr (std::is_same_v<T, sdw::Int>)
		// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
		else
			abort();
	}

	return sdw::FragmentWriter::declOutput<T>(name, location, enabled);
}

template <typename T>
inline sdw::Optional<T> FragmentWriter::declOutput(std::string const& name,
												   uint32_t location,
												   uint32_t attributes,
												   bool enabled)
{
	if (enabled)
	{
		if constexpr (std::is_same_v<T, sdw::Float>)
			m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::UInt>)
			m_outputAttachments[location] = VK_FORMAT_R32_UINT;
		// else if constexpr (std::is_same_v<T, sdw::Int>)
		// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
		else
			abort();
	}

	return sdw::FragmentWriter::declOutput<T>(name, location, attributes,
											  enabled);
}

template <typename T>
inline sdw::Optional<sdw::Array<T>> FragmentWriter::declOutputArray(
	std::string const& name, uint32_t location, uint32_t dimension,
	bool enabled)
{
	if (enabled)
	{
		if constexpr (std::is_same_v<T, sdw::Float>)
			m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::UInt>)
			m_outputAttachments[location] = VK_FORMAT_R32_UINT;
		// else if constexpr (std::is_same_v<T, sdw::Int>)
		// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
		else
			abort();
	}

	return sdw::FragmentWriter::declOutputArray<T>(name, location, dimension,
												   enabled);
}

template <typename T>
inline sdw::Optional<sdw::Array<T>> FragmentWriter::declOutputArray(
	std::string const& name, uint32_t location, uint32_t dimension,
	uint32_t attributes, bool enabled)
{
	if (enabled)
	{
		if constexpr (std::is_same_v<T, sdw::Float>)
			m_outputAttachments[location] = VK_FORMAT_R32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec2>)
			m_outputAttachments[location] = VK_FORMAT_R32G32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec3>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::Vec4>)
			m_outputAttachments[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
		else if constexpr (std::is_same_v<T, sdw::UInt>)
			m_outputAttachments[location] = VK_FORMAT_R32_UINT;
		// else if constexpr (std::is_same_v<T, sdw::Int>)
		// 	m_outputAttachments[location] = VK_FORMAT_R32_INT;
		else
			abort();
	}

	return sdw::FragmentWriter::declOutputArray<T>(name, location, dimension,
												   attributes, enabled);
}
/**@}*/
#pragma endregion

inline const std::vector<std::pair<uint32_t, VkDescriptorSetLayoutBinding>>&
ComputeWriter::getDescriptors() const
{
	return m_descriptors;
}

#pragma region Sampled Image declaration
/**
 *name
 *  Sampled Image declaration.
 */
/**@{*/
template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
          bool ArrayedT, bool DepthT, bool MsT>
inline sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>
ComputeWriter::declSampledImage(std::string const& name, uint32_t binding,
                                 uint32_t set)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::ComputeWriter::declSampledImage<FormatT, DimT, ArrayedT,
	                                             DepthT, MsT>(name, binding,
	                                                          set);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
          bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
ComputeWriter::declSampledImage(std::string const& name, uint32_t binding,
                                 uint32_t set, bool enabled)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::ComputeWriter::declSampledImage<FormatT, DimT, ArrayedT,
	                                             DepthT, MsT>(name, binding,
	                                                          set, enabled);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
          bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>
ComputeWriter::declSampledImageArray(std::string const& name,
                                      uint32_t binding, uint32_t set,
                                      uint32_t dimension)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = dimension;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::ComputeWriter::declSampledImageArray<FormatT, DimT, ArrayedT,
	                                                  DepthT, MsT>(
	    name, binding, set, dimension);
}

template <ast::type::ImageFormat FormatT, ast::type::ImageDim DimT,
          bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
    sdw::Array<sdw::SampledImageT<FormatT, DimT, ArrayedT, DepthT, MsT>>>
ComputeWriter::declSampledImageArray(std::string const& name,
                                      uint32_t binding, uint32_t set,
                                      uint32_t dimension, bool enabled)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = dimension;
	b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_descriptors.push_back({ set, b });

	return sdw::ComputeWriter::declSampledImageArray<FormatT, DimT, ArrayedT,
	                                                  DepthT, MsT>(
	    name, binding, set, dimension, enabled);
}
/**@}*/
#pragma endregion
#pragma region Image declaration
/**
 *name
 *  Image declaration.
 */
/**@{*/
template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>
ComputeWriter::declImage(std::string const& name, uint32_t binding,
                          uint32_t set)
{
	return sdw::ComputeWriter::declImage<FormatT, AccessT, DimT, ArrayedT,
	                                      DepthT, MsT>(name, binding, set);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
    sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
ComputeWriter::declImage(std::string const& name, uint32_t binding,
                          uint32_t set, bool enabled)
{
	return sdw::ComputeWriter::declImage<FormatT, AccessT, DimT, ArrayedT,
	                                      DepthT, MsT>(name, binding, set,
	                                                   enabled);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>
ComputeWriter::declImageArray(std::string const& name, uint32_t binding,
                               uint32_t set, uint32_t dimension)
{
	return sdw::ComputeWriter::declImageArray<FormatT, AccessT, DimT,
	                                           ArrayedT, DepthT, MsT>(
	    name, binding, set, dimension);
}

template <ast::type::ImageFormat FormatT, ast::type::AccessKind AccessT,
          ast::type::ImageDim DimT, bool ArrayedT, bool DepthT, bool MsT>
inline sdw::Optional<
    sdw::Array<sdw::ImageT<FormatT, AccessT, DimT, ArrayedT, DepthT, MsT>>>
ComputeWriter::declImageArray(std::string const& name, uint32_t binding,
                               uint32_t set, uint32_t dimension, bool enabled)
{
	return sdw::ComputeWriter::declImageArray<FormatT, AccessT, DimT,
	                                           ArrayedT, DepthT, MsT>(
	    name, binding, set, dimension, enabled);
}
/**@}*/
#pragma endregion
}  // namespace cdm
