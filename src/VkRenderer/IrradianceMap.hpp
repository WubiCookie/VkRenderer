#pragma once

#include "VulkanDevice.hpp"

#include "Cubemap.hpp"
#include "RenderWindow.hpp"
#include "Texture2D.hpp"

#include <string_view>

namespace cdm
{
class IrradianceMap final
{
	Cubemap m_irradianceMap;

public:
	IrradianceMap() = default;
	IrradianceMap(RenderWindow& renderWindow, uint32_t resolution,
	              Texture2D& equirectangularTexture,
	              std::string_view filePath);

	Cubemap& get() noexcept { return m_irradianceMap; }
	const Cubemap& get() const noexcept { return m_irradianceMap; }
};
}  // namespace cdm
