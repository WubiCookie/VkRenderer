#pragma once

#include "VulkanDevice.hpp"

#include "Cubemap.hpp"
#include "RenderWindow.hpp"

#include <string_view>

namespace cdm
{
class PrefilteredCubemap final
{
	Cubemap m_prefilteredCubemap;

public:
	PrefilteredCubemap() = default;
	PrefilteredCubemap(RenderWindow& renderWindow, uint32_t resolution,
	                   uint32_t mipLevels, Cubemap& cubemap,
	                   std::string_view filePath);

	Cubemap& get() noexcept { return m_prefilteredCubemap; }
	const Cubemap& get() const noexcept { return m_prefilteredCubemap; }
};
}  // namespace cdm
