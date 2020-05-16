#include "CommandBuffer.hpp"
//#include "Framebuffer.hpp"
#include "Mandelbulb.hpp"
#include "ShaderBall.hpp"
//#include "Material.hpp"
//#include "RenderPass.hpp"
#include "RenderWindow.hpp"
//#include "Renderer.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

//#define STB_IMAGE_WRITE_IMPLEMENTATION 1
//#include "stb_image_write.h"
#include "stb_image.h"

#include "EquirectangularToCubemap.hpp"
#include "EquirectangularToIrradianceMap.hpp"
#include "PrefilterCubemap.hpp"
#include "BrdfLutGenerator.hpp"

int main()
{
	using namespace cdm;

	RenderWindow rw(1280, 720, true);

	auto& vk = rw.device();
	vk.setLogActive();

	/*
	int w, h, c;
	float* imageData = stbi_loadf(
	    "D:/Projects/git/VkRenderer-data/PaperMill_Ruins_E/"
	    "PaperMill_E_3k.hdr",
	    &w, &h, &c, 4);

	if (!imageData)
		throw std::runtime_error("could not load equirectangular map");

	Texture2D m_equirectangularTexture = Texture2D(
	    rw, w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkBufferImageCopy copy{};
	copy.bufferRowLength = w;
	copy.bufferImageHeight = h;
	copy.imageExtent.width = w;
	copy.imageExtent.height = h;
	copy.imageExtent.depth = 1;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.mipLevel = 0;

	m_equirectangularTexture.uploadDataImmediate(
	    imageData, w * h * 4 * sizeof(float), copy, VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	stbi_image_free(imageData);

	cdm::UniqueFence fence = vk.createFence();

	//BrdfLutGenerator blg(rw, 1024);
	EquirectangularToIrradianceMap e2i(rw, 512);

	rw.acquireNextImage(fence.get());
	rw.present();
	vk.wait(fence.get());
	vk.resetFence(fence.get());

	//EquirectangularToCubemap e2c(rw, 1024);
	//Cubemap cubemap = e2c.computeCubemap(m_equirectangularTexture);

	//if (cubemap.get() == nullptr)
	//	throw std::runtime_error("could not create cubemap");

	//rw.acquireNextImage(fence.get());
	//rw.present();
	//vk.wait(fence.get());
	//vk.resetFence(fence.get());

	//PrefilterCubemap pfc(rw, 512, 5);
	//auto cm = pfc.computeCubemap(cubemap);

	//Texture2D lut = blg.computeBrdfLut();

	auto irr = e2i.computeCubemap(m_equirectangularTexture);

	rw.acquireNextImage(fence.get());
	rw.present();
	vk.wait(fence.get());
	vk.resetFence(fence.get());

	return 0;
	//*/

	{
		cdm::UniqueFence fence = vk.createFence();

		rw.acquireNextImage(fence.get());
		rw.present();

		// Mandelbulb mandelbulb(rw);
		ShaderBall shaderBall(rw);

		auto start = std::chrono::steady_clock::now();
		auto end = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		double deltaTime = elapsed_seconds.count();

		bool rightClicking = false;
		bool wPressed = false;
		bool aPressed = false;
		bool sPressed = false;
		bool dPressed = false;
		bool qPressed = false;
		bool ePressed = false;
		bool shouldClose = false;
		double previousX = 0.0;
		double previousY = 0.0;
		double currentX = 0.0;
		double currentY = 0.0;
		double deltaX = 0.0;
		double deltaY = 0.0;

		double rotationX = 0.0;
		double rotationY = 0.0;

		constexpr double mouseSpeed = 1.0;
		constexpr double mouseSpeedAttenuation = 0.01;
		constexpr double translationSpeed = 0.4;

		rw.registerMouseButtonCallback(
		    [&](MouseButton button, Action action, int mods) {
			    if (button == MouseButton::Right)
			    {
				    if (action == Action::Press)
					    rightClicking = true;
				    else if (action == Action::Release)
					    rightClicking = false;
			    }
		    });

		rw.registerMousePosCallback([&](double x, double y) {
			previousX = currentX;
			previousY = currentY;
			currentX = x;
			currentY = y;
			deltaX = previousX - currentX;
			deltaY = previousY - currentY;

			if (rightClicking)
			{
				double dx = std::min(10.0, (deltaX * mouseSpeed) *
				                               (deltaX * mouseSpeed) *
				                               mouseSpeedAttenuation);
				double dy = std::min(10.0, (deltaY * mouseSpeed) *
				                               (deltaY * mouseSpeed) *
				                               mouseSpeedAttenuation) *
				            (1280.0 / 720.0);  // balance aspect ratio

				if (deltaX > 0.0)
					rotationX += dx;
				else
					rotationX -= dx;

				if (deltaY > 0.0)
					rotationY += dy;
				else
					rotationY -= dy;
			}
		});

		rw.registerKeyCallback(
		    [&](Key key, int scancode, Action action, int mods) {
			    if (action == Action::Press)
			    {
				    if (key == Key::W)
					    wPressed = true;
				    if (key == Key::A)
					    aPressed = true;
				    if (key == Key::S)
					    sPressed = true;
				    if (key == Key::D)
					    dPressed = true;
				    if (key == Key::Q)
					    qPressed = true;
				    if (key == Key::E)
					    ePressed = true;

				    if (key == Key::Escape)
					    shouldClose = true;
			    }
			    else if (action == Action::Release)
			    {
				    if (key == Key::W)
					    wPressed = false;
				    if (key == Key::A)
					    aPressed = false;
				    if (key == Key::S)
					    sPressed = false;
				    if (key == Key::D)
					    dPressed = false;
				    if (key == Key::Q)
					    qPressed = false;
				    if (key == Key::E)
					    ePressed = false;
			    }
		    });

		while (!rw.shouldClose() && !shouldClose)
		{
			end = std::chrono::steady_clock::now();
			elapsed_seconds = end - start;
			deltaTime = elapsed_seconds.count();
			start = std::chrono::steady_clock::now();

			deltaX = 0.0;
			deltaY = 0.0;

			rw.pollEvents();

			shaderBall.cameraTr.rotation =
			    quaternion(vector3(0, 1, 0), degree(rotationX)) *
			    quaternion(vector3(1, 0, 0), degree(rotationY));

			if (rightClicking)
			{
				vector3 translation;

				if (wPressed)
					translation.z -= 1.0;
				if (aPressed)
					translation.x -= 1.0;
				if (sPressed)
					translation.z += 1.0;
				if (dPressed)
					translation.x += 1.0;
				if (qPressed)
					shaderBall.cameraTr.position.y -= translationSpeed;
				if (ePressed)
					shaderBall.cameraTr.position.y += translationSpeed;

				if (translation.norm() > 0.01f)
				{
					translation.normalize();
					translation *= translationSpeed;

					shaderBall.cameraTr.position +=
					    shaderBall.cameraTr.rotation * translation;
				}
			}

			vk.wait(fence.get());
			vk.resetFence(fence.get());

			// mandelbulb.standaloneDraw();
			shaderBall.standaloneDraw();

			rw.acquireNextImage(fence.get());
			rw.present();
		}

		vk.wait(fence.get());
		vk.wait(vk.graphicsQueue());
		vk.wait();
	}

	vk.setLogInactive();
}
