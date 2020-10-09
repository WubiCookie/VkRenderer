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
void LightTransport::createDescriptorsObjects()
{
	auto& vk = rw.get().device();

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 16 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 16;
	poolInfo.poolSizeCount = uint32_t(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	m_descriptorPool = vk.create(poolInfo);
	if (!m_descriptorPool)
	{
		std::cerr << "error: failed to create descriptor pool" << std::endl;
		abort();
	}
#pragma endregion

	/*
#pragma region descriptor set
	{
		m_descriptorSet = vk.allocate(m_descriptorPool, m_setLayouts[0]);

		if (!m_descriptorSet)
		{
			std::cerr << "error: failed to allocate descriptor set"
			          << std::endl;
			abort();
		}
	}
#pragma endregion
	//*/

#pragma region blit descriptor set
	m_blitDescriptorSet = vk.allocate(m_descriptorPool, m_blitSetLayouts[0]);

	if (!m_blitDescriptorSet)
	{
		std::cerr << "error: failed to allocate blit descriptor set"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region trace descriptor set
	m_traceDescriptorSet = vk.allocate(m_descriptorPool, m_traceSetLayouts[0]);

	if (!m_traceDescriptorSet)
	{
		std::cerr << "error: failed to allocate trace descriptor set"
		          << std::endl;
		abort();
	}
#pragma endregion
}
}  // namespace cdm
