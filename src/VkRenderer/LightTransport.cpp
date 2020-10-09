#include "LightTransport.hpp"
#include "cdm_maths.hpp"

#include <CompilerSpirV/compileSpirV.hpp>
#include <ShaderWriter/Intrinsics/Intrinsics.hpp>
#include <ShaderWriter/Source.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "my_imgui_impl_vulkan.h"

#include <array>
#include <iostream>
#include <stdexcept>

constexpr size_t POINT_COUNT = 2000;

#define TEX_SCALE 1

constexpr uint32_t width = 1280 * TEX_SCALE;
constexpr uint32_t height = 720 * TEX_SCALE;
constexpr float widthf = 1280.0f * TEX_SCALE;
constexpr float heightf = 720.0f * TEX_SCALE;

bool reset = true;

namespace cdm
{
void LightTransport::Config::copyTo(void* ptr)
{
	std::memcpy(ptr, this, sizeof(*this));
}

void LightTransport::PushConstants::addPcbMembers(sdw::Pcb& pcb)
{
	pcb.declMember<sdw::Int>("init");
	pcb.declMember<sdw::Float>("rng");
	pcb.declMember<sdw::Float>("smoothness");
	pcb.declMember<sdw::UInt>("bumpsCount");
	pcb.end();
}

sdw::Int LightTransport::PushConstants::getInit(sdw::Pcb& pcb)
{
	return pcb.getMember<sdw::Int>("init");
}

sdw::Float LightTransport::PushConstants::getRng(sdw::Pcb& pcb)
{
	return pcb.getMember<sdw::Float>("rng");
}

sdw::Float LightTransport::PushConstants::getSmoothness(sdw::Pcb& pcb)
{
	return pcb.getMember<sdw::Float>("smoothness");
}

sdw::UInt LightTransport::PushConstants::getBumpsCount(sdw::Pcb& pcb)
{
	return pcb.getMember<sdw::UInt>("bumpsCount");
}

VkPushConstantRange LightTransport::PushConstants::getRange()
{
	return { VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 0,
		     sizeof(*this) };
}

static float smoothness = 0.01f;

LightTransport::LightTransport(RenderWindow& renderWindow)
    : rw(renderWindow),
      rd(),
      gen(rd())
{
	auto& vk = rw.get().device();

	vk.setLogActive();

	// m_config.spherePos.x = 67.0f;
	// m_config.spherePos.y = 357.0f;
	// m_config.sphereRadius = 41.0f;
	m_config.spherePos.x = widthf / 2.0f;
	m_config.spherePos.y = heightf / 2.0f;
	m_config.sphereRadius = 200.0f;

	m_config.deltaT = 0.5f;
	m_config.sphereRefraction = 0.7f;

	createRenderPasses();
	createShaderModules();
	createDescriptorsObjects();
	createPipelines();
	createBuffers();
	createImages();
	createFramebuffers();
	updateDescriptorSets();

	vk.setLogInactive();

	/*
	auto& frame = rw.get().getAvailableCommandBuffer();
	frame.reset();

	auto& cb = frame.commandBuffer;

	cb.reset();
	cb.begin();
	cb.debugMarkerBegin("compute", 0.7f, 1.0f, 0.6f);

	cb.bindPipeline(m_tracePipeline);
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_tracePipelineLayout,
	0, m_traceDescriptorSet);

	std::uniform_real_distribution<float> urd(0.0f, 1.0f);
	m_pc.init = 1;
	m_pc.rng = urd(gen);
	m_pc.smoothness = smoothness;
	m_pc.bumpsCount = BUMPS;
	cb.pushConstants(m_tracePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
	&m_pc);

	//int32_t i = 1;
	//cb.pushConstants(m_tracePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
	&i);
	//
	//std::uniform_real_distribution<float> urd(0.0f, 1.0f);
	//float rng = urd(gen);
	//cb.pushConstants(m_tracePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
	sizeof(int32_t), &rng);
	//cb.pushConstants(m_tracePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
	sizeof(int32_t) + sizeof(float), &smoothness);

	cb.dispatch(VERTEX_BUFFER_LINE_COUNT / THREAD_COUNT);

	cb.debugMarkerEnd();
	cb.end();
	if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
	{
	    std::cerr << "error: failed to submit compute command buffer"
	                << std::endl;
	    abort();
	}
	vk.wait(vk.graphicsQueue());
	//*/
}

LightTransport::~LightTransport() {}

static int i = VERTEX_BATCH_COUNT;
static int b = VERTEX_BATCH_COUNT + 1;
static int j = 0;

void LightTransport::render(CommandBuffer& cb)
{
	// vk::Viewport viewport;
	// viewport.width = widthf;
	// viewport.height = heightf;
	// viewport.minDepth = 0.0f;
	// viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.extent.width = width;
	scissor.extent.height = height;

	std::array clearValues = { VkClearValue{}, VkClearValue{} };
	// clearValues[0].color.float32[0] = 0x27 / 255.0f;
	// clearValues[0].color.float32[1] = 0x28 / 255.0f;
	// clearValues[0].color.float32[2] = 0x22 / 255.0f;
	// clearValues[0].color.float32[3] = 1.0f;

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_framebuffer;
	rpInfo.renderPass = m_renderPass;
	rpInfo.renderArea.extent = scissor.extent;
	// rpInfo.renderArea.extent.width = width;
	// rpInfo.renderArea.extent.height = height;
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	if (b)
	{
		if (i > 0)
		{
			fillRaysBuffer();

			cb.debugMarkerBegin("compute", 0.7f, 1.0f, 0.6f);
			cb.bindPipeline(m_tracePipeline);
			cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
			                     m_tracePipelineLayout, 0,
			                     m_traceDescriptorSet);

			std::uniform_real_distribution<float> urd(0.0f, 1.0f);
			m_pc.init = 0;
			m_pc.rng = urd(gen);
			m_pc.smoothness = smoothness;
			m_pc.bumpsCount = BUMPS;
			cb.pushConstants(m_tracePipelineLayout,
			                 VK_SHADER_STAGE_COMPUTE_BIT, 0, &m_pc);

			// int32_t k = 0;
			// cb.pushConstants(m_tracePipelineLayout,
			// VK_SHADER_STAGE_COMPUTE_BIT, 0, &k);
			// std::uniform_real_distribution<float> urd(0.0f, 1.0f);
			// float rng = urd(gen);
			// cb.pushConstants(m_tracePipelineLayout,
			// VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t), &rng);
			// cb.pushConstants(m_tracePipelineLayout,
			// VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) + sizeof(float),
			// &smoothness);
			cb.dispatch(VERTEX_BUFFER_LINE_COUNT / THREAD_COUNT);
			// cb.dispatch(1);
			cb.debugMarkerEnd();

			i--;
			j++;
		}

		rpInfo.renderArea.extent.width = width * HDR_SCALE;
		rpInfo.renderArea.extent.height = height * HDR_SCALE;
		cb.beginRenderPass2(rpInfo, subpassBeginInfo);

		cb.bindPipeline(m_pipeline);
		// cb.setViewport(viewport);
		// cb.setScissor(scissor);
		cb.bindVertexBuffer(m_vertexBuffer);
		// cb.draw(POINT_COUNT);
		cb.draw(VERTEX_BUFFER_LINE_COUNT * 2 * BUMPS);

		cb.endRenderPass2(subpassEndInfo);

		b--;
	}

	rpInfo.framebuffer = m_blitFramebuffer;
	rpInfo.renderPass = m_blitRenderPass;
	rpInfo.renderArea.extent.width = width;
	rpInfo.renderArea.extent.height = height;
	// rpInfo.renderArea.extent.width = width;
	// rpInfo.renderArea.extent.height = height;
	// rpInfo.clearValueCount = 1;
	// rpInfo.pClearValues = clearValues.data();

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	cb.bindPipeline(m_blitPipeline);
	// cb.setViewport(viewport);
	// cb.setScissor(scissor);
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_blitPipelineLayout,
	                     0, m_blitDescriptorSet);
	// cb.bindVertexBuffer(m_vertexBuffer);
	// cb.draw(POINT_COUNT);

	// float exposure = widthf / std::max(1.0f, float(j) *
	// float(VERTEX_BUFFER_LINE_COUNT) * float(VERTEX_BATCH_COUNT)); float
	// exposure = widthf / (float(j) * float(VERTEX_BUFFER_LINE_COUNT)); float
	// exposure = 1.0f / (float(j) * float(VERTEX_BUFFER_LINE_COUNT));
	float exposure = 1.0f / (j * float(VERTEX_BUFFER_LINE_COUNT) / 8);
	// float exposure = widthf / std::max(1.0f, float(j) *
	// float(VERTEX_BATCH_COUNT) * float(4)); float exposure = widthf /
	// std::max(1.0f, float(j) * float(VERTEX_BATCH_COUNT));

	cb.pushConstants(m_blitPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
	                 &exposure);

	cb.draw(3);

	cb.endRenderPass2(subpassEndInfo);
}

void LightTransport::compute(CommandBuffer& cb)
{
	// cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
	//                      m_rayComputePipelineLayout, 0, m_rayComputeSet);
	// cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
	//                      m_colorComputePipelineLayout, 1,
	//                      m_colorComputeSet);
	// cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_rayComputePipeline);
	// cb.dispatch(RAYS_COUNT / THREAD_COUNT);
	//
	// vk::BufferMemoryBarrier barrier;
	// barrier.buffer = m_raysBuffer;
	// barrier.size = sizeof(RayIteration) * RAYS_COUNT;
	// barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	// barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	// cb.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	//                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, barrier);
	//
	// cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_colorComputePipeline);
	// cb.dispatch(width / 8, height / 8);
}

void LightTransport::imgui(CommandBuffer& cb)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Controls");

		bool changed = false;

		changed |= ImGui::SliderFloat("smoothness", &smoothness, 0.001f, 0.5f,
		                              "%.3f", 1.2f);
		changed |=
		    ImGui::SliderFloat("angle", &m_angle, -constants<float>::Pi(),
		                       constants<float>::Pi(), "%.3f");
		changed |= ImGui::SliderFloat("aperture", &m_aperture, 0.0f,
		                              constants<float>::Pi(), "%.3f");
		changed |=
		    ImGui::SliderFloat("sourceSize", &m_sourceSize, 0.0f, 50.0f);

		// changed |=
		//    ImGui::SliderFloat("sphereRefraction",
		//    &m_config.sphereRefraction,
		//                       0.001f, 2.0f, "%.3f", 1.2f);

		// changed |= ImGui::SliderFloat("airRefractionScale",
		//                              &m_config.airRefractionScale, 0.0f,
		//                              20.0f, "%.3f", 2.2f);
		// changed |= ImGui::SliderFloat("sphereRefractionScale",
		//                              &m_config.sphereRefractionScale, 0.0f,
		//                              20.0f, "%.3f", 2.2f);

		// changed |= ImGui::DragFloat2("spherePos",
		// &m_config.spherePos.x, 1.0f); changed |=
		// ImGui::SliderFloat("sphereRadius", &m_config.sphereRadius,
		//                              0.0f, 200.0f);

		// changed |= ImGui::SliderFloat("deltaT", &m_config.deltaT,
		// 0.001f, 2.0f,
		//                              "%.3f", 1.2f);

		// changed |= ImGui::SliderFloat("raySize", &m_config.raySize, 0.001f,
		//                              5.0f, "%.3f", 1.2f);

		reset |= ImGui::Button("reset");
		changed |= reset;

		// changed |= ImGui::SliderFloat("CamFocalDistance",
		//                              &m_config.camFocalDistance,
		//                              0.1f, 30.0f);
		// changed |= ImGui::SliderFloat("CamFocalLength",
		//                              &m_config.camFocalLength, 0.0f, 20.0f);
		// changed |= ImGui::SliderFloat("CamAperture", &m_config.camAperture,
		//                              0.0f, 5.0f);
		//
		// changed |= ImGui::DragFloat3("rotation", &m_config.camRot.x, 0.01f);
		// changed |= ImGui::DragFloat3("lightDir", &m_config.lightDir.x,
		// 0.01f);
		//
		// changed |= ImGui::SliderFloat("scene radius", &m_config.sceneRadius,
		//                              0.0f, 10.0f);
		//
		// ImGui::SliderFloat("BloomAscale1", &m_config.bloomAscale1,
		// 0.0f, 1.0f); ImGui::SliderFloat("BloomAscale2",
		// &m_config.bloomAscale2, 0.0f, 1.0f);
		// ImGui::SliderFloat("BloomBscale1", &m_config.bloomBscale1,
		// 0.0f, 1.0f); ImGui::SliderFloat("BloomBscale2",
		// &m_config.bloomBscale2, 0.0f, 1.0f);
		//
		// changed |= ImGui::SliderFloat("Power", &m_config.power,
		// 0.0f, 50.0f); changed |= ImGui::DragInt("Iterations",
		// &m_config.iterations, 0.1f, 1, 16);

		if (changed)
			applyImguiParameters();

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
		            1000.0f / ImGui::GetIO().Framerate,
		            ImGui::GetIO().Framerate);
		ImGui::End();
	}

	ImGui::Render();

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_outputImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
	                   barrier);

	{
		std::array clearValues = { VkClearValue{}, VkClearValue{} };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.framebuffer = m_blitFramebuffer.get();
		rpInfo.renderPass = rw.get().imguiRenderPass();
		rpInfo.renderArea.extent.width = width;
		rpInfo.renderArea.extent.height = height;
		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues = clearValues.data();

		vk::SubpassBeginInfo subpassBeginInfo;
		subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

		cb.beginRenderPass2(rpInfo, subpassBeginInfo);
	}

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb.get());

	vk::SubpassEndInfo subpassEndInfo2;
	cb.endRenderPass2(subpassEndInfo2);

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
	                   barrier);
}

void LightTransport::standaloneDraw()
{
	auto& vk = rw.get().device();

	auto& frame = rw.get().getAvailableCommandBuffer();
	frame.reset();

	auto& cb = frame.commandBuffer;

	//*
	if (reset == true)
	{
		reset = false;

		// const float waveStep = (750.0f - 360.0f) / float(RAYS_COUNT);
		// const float freqStep = (7.9e14f - 4.05e14f) / float(RAYS_COUNT);
		//// float wave = 400.0f;
		//// float freq = 4.05e14f;
		// float freq = 5.9e14f;
		// std::vector<RayIteration> raysVector(RAYS_COUNT);
		// for (size_t i = 0; i < RAYS_COUNT; i++)
		//{
		//	raysVector[i] = RayIteration{};
		//	raysVector[i].position = {
		//		1000.0f, heightf / 2.0f + (i / float(RAYS_COUNT - 1)) * 80.0f
		//	};
		//	// raysVector[i].position = {
		//	//	10.0f, heightf / 2.0f
		//	//};
		//	raysVector[i].direction = { -1.0f, 0.0f };
		//	// raysVector[i].waveLength = wave;
		//	raysVector[i].currentRefraction = m_config.airRefraction;
		//	// wave += waveStep;
		//
		//	raysVector[i].frequency = freq;
		//
		//	raysVector[i].waveLength = 3.0e8f * 1.0e9f /
		//raysVector[i].frequency;
		//
		//	// freq += freqStep;
		//}
		//
		// auto setDir = [&](size_t i) {
		//	vector2 p = { raysVector[i].position.x, raysVector[i].position.y };
		//	vector2 c = { widthf / 2.0f, heightf / 2.0f };
		//	vector2 d = vector2::from_to(p, c).get_normalized();
		//	raysVector[i].direction.x = d.x;
		//	raysVector[i].direction.y = d.y;
		//};
		//
		// raysVector[0].position = { 0, 0 };
		// raysVector[1].position = { widthf / 2.0f, 0 };
		// raysVector[2].position = { widthf, 0 };
		//
		// raysVector[3].position = { 0, heightf / 2.0f };
		//
		// raysVector[4].position = { widthf, heightf / 2.0f };
		//
		// raysVector[5].position = { 0, heightf };
		// raysVector[6].position = { widthf / 2.0f, heightf };
		// raysVector[7].position = { widthf, heightf };
		//
		// for (size_t i = 0; i < RAYS_COUNT; i++)
		//	setDir(i);
		//
		// RayIteration* rayPtr = m_raysBuffer.map<RayIteration>();
		// std::memcpy(rayPtr, raysVector.data(),
		//            sizeof(*raysVector.data()) * RAYS_COUNT);
		// m_raysBuffer.unmap();

		fillRaysBuffer();

		i = VERTEX_BATCH_COUNT;
		b = VERTEX_BATCH_COUNT + 1;
		j = 0;

		/*
		{
		    auto& frame = rw.get().getAvailableCommandBuffer();
		    frame.reset();

		    auto& cb = frame.commandBuffer;

		    cb.reset();
		    cb.begin();
		    cb.debugMarkerBegin("compute", 0.7f, 1.0f, 0.6f);

		    cb.bindPipeline(m_tracePipeline);
		    cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
		m_tracePipelineLayout, 0, m_traceDescriptorSet);

		    //int32_t i = 1;
		    //cb.pushConstants(m_tracePipelineLayout,
		VK_SHADER_STAGE_COMPUTE_BIT, 0, &i);

		    //std::uniform_real_distribution<float> urd(0.0f, 1.0f);
		    //float rng = urd(gen);
		    //cb.pushConstants(m_tracePipelineLayout,
		VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t), &rng);
		    //cb.pushConstants(m_tracePipelineLayout,
		VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) + sizeof(float),
		&smoothness);

		    std::uniform_real_distribution<float> urd(0.0f, 1.0f);
		    m_pc.init = 1;
		    m_pc.rng = urd(gen);
		    m_pc.smoothness = smoothness;
		    m_pc.bumpsCount = BUMPS;
		    cb.pushConstants(m_tracePipelineLayout,
		VK_SHADER_STAGE_COMPUTE_BIT, 0, &m_pc);

		    cb.dispatch(VERTEX_BUFFER_LINE_COUNT / THREAD_COUNT);

		    cb.debugMarkerEnd();
		    cb.end();
		    if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
		    {
		        std::cerr << "error: failed to submit compute command buffer"
		                    << std::endl;
		        abort();
		    }
		    vk.wait(vk.graphicsQueue());
		}
		//*/

		cb.reset();
		cb.begin();
		cb.debugMarkerBegin("reset", 1.0f, 0.7f, 0.6f);

		auto prop = vk.getPhysicalDeviceFormatProperties(
		    VK_FORMAT_R32G32B32A32_SFLOAT);
		std::cout << "\nlinear:  "
		          << vk::feature_to_string(prop.linearTilingFeatures)
		          << std::endl;
		std::cout << "optimal: "
		          << vk::feature_to_string(prop.optimalTilingFeatures)
		          << std::endl;

		Texture2D blackImage(
		    rw, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_LINEAR,
		    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		        VK_IMAGE_USAGE_SAMPLED_BIT,
		    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		std::array<float, 4> pixel{ 0.0f };

		VkBufferImageCopy region{};
		region.bufferImageHeight = 1;
		region.imageExtent.width = 1;
		region.imageExtent.height = 1;
		region.imageExtent.depth = 1;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageSubresource.mipLevel = 0;

		blackImage.uploadDataImmediate(pixel.data(), sizeof(float) * 4, region,
		                               VK_IMAGE_LAYOUT_UNDEFINED,
		                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		m_outputImageHDR.transitionLayoutImmediate(
		    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkImageBlit blit{};
		blit.srcOffsets[1].x = 1;
		blit.srcOffsets[1].y = 1;
		blit.srcOffsets[1].z = 1;
		blit.dstOffsets[1].x = m_outputImageHDR.width();
		blit.dstOffsets[1].y = m_outputImageHDR.height();
		blit.dstOffsets[1].z = m_outputImageHDR.depth();
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = 0;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.mipLevel = 0;

		cb.blitImage(blackImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		             m_outputImageHDR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		             blit, VK_FILTER_LINEAR);

		cb.debugMarkerEnd();
		cb.end();
		if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit reset command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(vk.graphicsQueue());

		m_outputImageHDR.transitionLayoutImmediate(
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	}
	//*/

	// cb.reset();
	// cb.begin();
	// cb.debugMarkerBegin("compute", 1.0f, 0.2f, 0.2f);
	// compute(cb);
	// cb.debugMarkerEnd();
	// cb.end();
	// if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
	//{
	//	std::cerr << "error: failed to submit imgui command buffer"
	//	          << std::endl;
	//	abort();
	//}
	// vk.wait(vk.graphicsQueue());

	// outputTextureHDR().generateMipmapsImmediate(VK_IMAGE_LAYOUT_GENERAL);

	cb.reset();
	cb.begin();
	cb.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
	render(cb);
	imgui(cb);
	cb.debugMarkerEnd();
	cb.end();
	if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit render/imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	frame.reset();

	rw.get().present(m_outputImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr);

	// setSampleAndRandomize(0.0f);
}

void LightTransport::applyImguiParameters()
{
	auto& vk = rw.get().device();

	reset = true;
}

void LightTransport::randomizePoints()
{
	// auto& vk = rw.get().device();
	//
	// using Vertex = std::array<float, 2>;
	// std::vector<Vertex> vertices(POINT_COUNT);
	//
	// for (auto& vertex : vertices)
	//{
	//	vertex[0] = dis(gen);
	//	vertex[1] = dis(gen);
	//}
	//
	// Vertex* data = m_vertexBuffer.map<Vertex>();
	// std::copy(vertices.begin(), vertices.end(), static_cast<Vertex*>(data));
	// m_vertexBuffer.unmap();
	//
	// m_config.seed = udis(gen);
	// m_config.copyTo(m_computeUbo.map());
	// m_computeUbo.unmap();
}

void LightTransport::fillVertexBuffer()
{
	std::uniform_real_distribution<float> urd(0.0f,
	                                          2.0f * constants<float>::Pi());
	std::uniform_real_distribution<float> urdcol(0.0f, 1.0f);

	std::vector<Line> lines(VERTEX_BUFFER_LINE_COUNT);
	for (auto& line : lines)
	{
		radian theta(urd(gen) / 8.0f + 3.14f - (3.14f / 8.0f));
		vector2 dir{ cos(theta), sin(theta) };

		line.A.pos = { 0.5, 0 };
		line.B.pos = dir * 2.0f + line.A.pos;
		line.A.col.x = urdcol(gen);
		line.A.col.y = urdcol(gen);
		line.A.col.z = urdcol(gen);
		line.B.col = line.A.col;

		dir = line.B.pos - line.A.pos;
		dir.normalize();

		line.A.dir = dir;
		line.B.dir = dir;
	}

	Line* data = m_vertexBuffer.map<Line>();
	std::copy(lines.begin(), lines.end(), data);
	m_vertexBuffer.unmap();
}

void LightTransport::fillRaysBuffer()
{
	std::uniform_real_distribution<float> urd(0.0f,
	                                          2.0f * constants<float>::Pi());
	std::uniform_real_distribution<float> urdcol(0.0f, 1.0f);
	// std::uniform_real_distribution<float> urd(-1.0f, 1.0f);
	std::uniform_real_distribution<float> urdAngle(-m_aperture, m_aperture);

	// std::normal_distribution<float> urd(1.0f, 1.0f);

	std::vector<RayIteration> rays(VERTEX_BUFFER_LINE_COUNT);
	for (auto& ray : rays)
	{
		radian theta(urdAngle(gen) + m_angle);
		// radian theta(urd(gen));
		vector2 dir{ cos(theta), sin(theta) };

		radian alpha(urd(gen));
		float r = urdcol(gen);

		// ray.position = { 1, height / 2.0f };
		ray.position = { widthf / 2.0f + cos(alpha) * r * m_sourceSize,
			             height / 2.0f + sin(alpha) * r * m_sourceSize };
		ray.direction.x = dir.x;
		ray.direction.y = dir.y;
		ray.polarDirection = theta;
		ray.rng = urdcol(gen);
		// ray.frequency = urdcol(gen);
	}

	RayIteration* data = m_raysBuffer.map<RayIteration>();
	std::copy(rays.begin(), rays.end(), data);
	m_raysBuffer.unmap();
}

void LightTransport::setSampleAndRandomize(float s)
{
	// fillVertexBuffer();
}
}  // namespace cdm
