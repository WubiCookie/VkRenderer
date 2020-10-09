#include "LightTransport.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

constexpr size_t POINT_COUNT = 2000;

#define TEX_SCALE 1

constexpr uint32_t width = 1280 * TEX_SCALE;
constexpr uint32_t height = 720 * TEX_SCALE;
constexpr float widthf = 1280.0f * TEX_SCALE;
constexpr float heightf = 720.0f * TEX_SCALE;

namespace cdm
{
void LightTransport::updateDescriptorSets()
{
	auto& vk = rw.get().device();

#pragma region trace compute descriptors
	{
		VkDescriptorBufferInfo raysBufferInfo{};
		raysBufferInfo.buffer = m_raysBuffer;
		raysBufferInfo.range = sizeof(RayIteration) * VERTEX_BUFFER_LINE_COUNT;
		raysBufferInfo.offset = 0;

		vk::WriteDescriptorSet write1;
		write1.descriptorCount = 1;
		write1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write1.dstArrayElement = 0;
		write1.dstBinding = 0;
		write1.dstSet = m_traceDescriptorSet;
		write1.pBufferInfo = &raysBufferInfo;

		VkDescriptorBufferInfo verticesBufferInfo{};
		verticesBufferInfo.buffer = m_vertexBuffer;
		verticesBufferInfo.range = sizeof(Line) * VERTEX_BUFFER_LINE_COUNT * BUMPS;
		verticesBufferInfo.offset = 0;

		vk::WriteDescriptorSet write2;
		write2.descriptorCount = 1;
		write2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write2.dstArrayElement = 0;
		write2.dstBinding = 1;
		write2.dstSet = m_traceDescriptorSet;
		write2.pBufferInfo = &verticesBufferInfo;

		vk.updateDescriptorSets({write1, write2});
	}
#pragma endregion

	/*
#pragma region color compute UBO
	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_computeUbo.get();
	setBufferInfo.range = sizeof(m_config);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 1;
	uboWrite.dstSet = m_rayComputeSet.get();
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion
	//*/

#pragma region outputImageHDR
	VkDescriptorImageInfo setImageInfo =
	    m_outputImageHDR.makeDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL);

	// setImageInfo.imageView = m_outputImageHDR.view();
	// setImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	//VkDescriptorImageInfo setImageInfo2 =
	//    m_outputImageHDR.makeDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL);
	// setImageInfo2.imageView = m_outputImageHDR.view();
	// setImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	// setImageInfo2.sampler = m_outputImageHDR.sampler();

	vk::WriteDescriptorSet write;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.dstArrayElement = 0;
	write.dstBinding = 0;
	write.dstSet = m_blitDescriptorSet;
	write.pImageInfo = &setImageInfo;

	//vk::WriteDescriptorSet write2;
	//write2.descriptorCount = 1;
	//write2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//write2.dstArrayElement = 0;
	//write2.dstBinding = 1;
	//write2.dstSet = m_colorComputeSet.get();
	//// write2.pImageInfo = &setImageInfo2;
	//write2.pImageInfo = &setImageInfo;

	// std::array writes{ write, write2 };
	// vk.updateDescriptorSets(uint32_t(writes.size()), writes.data());

	//vk.updateDescriptorSets({ write, write2 });
	vk.updateDescriptorSets(write);
#pragma endregion

	/*
#pragma region waveLengthTransferFunction
	VkDescriptorImageInfo waveLengthTransferFunctionImageInfo =
	    m_waveLengthTransferFunction.makeDescriptorInfo(
	        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	//waveLengthTransferFunctionImageInfo.imageView =
	//    m_waveLengthTransferFunction.view();
	//waveLengthTransferFunctionImageInfo.imageLayout =
	//    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//waveLengthTransferFunctionImageInfo.sampler =
	//    m_waveLengthTransferFunction.sampler();

	vk::WriteDescriptorSet waveLengthTransferFunctionWrite;
	waveLengthTransferFunctionWrite.descriptorCount = 1;
	waveLengthTransferFunctionWrite.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	waveLengthTransferFunctionWrite.dstArrayElement = 0;
	waveLengthTransferFunctionWrite.dstBinding = 2;
	waveLengthTransferFunctionWrite.dstSet = m_colorComputeSet.get();
	waveLengthTransferFunctionWrite.pImageInfo =
	    &waveLengthTransferFunctionImageInfo;

	vk.updateDescriptorSets(waveLengthTransferFunctionWrite);
#pragma endregion
	//*/
}
}  // namespace cdm
