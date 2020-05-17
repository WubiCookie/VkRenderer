#pragma once

#include "VulkanDevice.hpp"

#include "RenderWindow.hpp"
#include "Texture2D.hpp"

#include <string_view>

namespace cdm
{
class BrdfLut final
{
	Texture2D m_brdfLut;

public:
	BrdfLut() = default;
	BrdfLut(RenderWindow& renderWindow, uint32_t resolution = 1024,
	        std::string_view filePath = "brdfLut.hdr");

	Texture2D& get() noexcept { return m_brdfLut; }
	const Texture2D& get() const noexcept { return m_brdfLut; }
};
}  // namespace cdm
