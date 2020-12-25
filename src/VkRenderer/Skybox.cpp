#include "Skybox.hpp"

#include "BrdfLutGenerator.hpp"
#include "EquirectangularToCubemap.hpp"
#include "EquirectangularToIrradianceMap.hpp"
#include "PrefilterCubemap.hpp"
#include "StagingBuffer.hpp"

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#include "stb_image.h"

#include <array>
#include <iostream>
#include <stdexcept>

namespace cdm
{
void Skybox::Config::copyTo(void* ptr)
{
    std::memcpy(ptr, this, sizeof(*this));
}

Skybox::Skybox(RenderWindow& renderWindow, VkRenderPass renderPass,
               VkViewport viewport, Cubemap& cubemap)
    : rw(renderWindow),
      m_renderPass(renderPass),
      m_viewport(viewport),
      m_cubemap(cubemap)
{
    auto& vk = rw.get().device();

    LogRRID log(vk);

#define Locale(name, value) auto name = writer.declLocale(#name, value);
#define FLOAT(name) auto name = writer.declLocale<Float>(#name);
#define VEC2(name) auto name = writer.declLocale<Vec2>(#name);
#define VEC3(name) auto name = writer.declLocale<Vec3>(#name);
#define Constant(name, value) auto name = writer.declConstant(#name, value);

#pragma region vertexShader
    std::cout << "vertexShader" << std::endl;
    {
        using namespace sdw;
        VertexWriter writer;

        auto inPosition = writer.declInput<Vec3>("inPosition", 0);

        auto fragPosition = writer.declOutput<Vec3>("fragPosition", 0);

        auto out = writer.getOut();

        Ubo ubo(writer, "ubo", 0, 0);
        ubo.declMember<Mat4>("view");
        ubo.declMember<Mat4>("proj");
        ubo.end();

        writer.implementMain([&]() {
            auto view = ubo.getMember<Mat4>("view");
            auto proj = ubo.getMember<Mat4>("proj");

            fragPosition = inPosition;

            Locale(rotView,
                   mat4(vec4(view[0][0], view[0][1], view[0][2], 0.0_f),
                        vec4(view[1][0], view[1][1], view[1][2], 0.0_f),
                        vec4(view[2][0], view[2][1], view[2][2], 0.0_f),
                        vec4(0.0_f, 0.0_f, 0.0_f, 1.0_f)));

            Locale(clipPos, proj * rotView * vec4(inPosition, 1.0_f));

            out.vtx.position = clipPos.xyww();
        });

        std::vector<uint32_t> bytecode =
            spirv::serialiseSpirv(writer.getShader());

        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
        createInfo.pCode = bytecode.data();

        m_vertexModule = vk.create(createInfo);
        if (!m_vertexModule)
        {
            std::cerr << "error: failed to create vertex shader module"
                      << std::endl;
            abort();
        }
    }
#pragma endregion

#pragma region fragmentShader
    std::cout << "fragmentShader" << std::endl;
    {
        using namespace sdw;
        FragmentWriter writer;

        auto in = writer.getIn();

        auto fragPosition = writer.declInput<Vec3>("fragPosition", 0);

        auto fragColor = writer.declOutput<Vec4>("fragColor", 0);
        auto fragID = writer.declOutput<UInt>("fragID", 1);
        auto fragNormalDepth = writer.declOutput<Vec4>("fragNormalDepth", 2);
        auto fragPos = writer.declOutput<Vec3>("fragPos", 3);

        auto environmentMap =
            writer.declSampledImage<FImgCubeRgba32>("environmentMap", 1, 0);

        Ubo ubo(writer, "ubo", 0, 0);
        ubo.declMember<Mat4>("view");
        ubo.declMember<Mat4>("proj");
        ubo.end();

        writer.implementMain([&]() {
            Locale(envColor, environmentMap.lod(fragPosition, 0.0_f));

            envColor = envColor / (envColor + vec4(1.0_f));
            envColor = pow(envColor, vec4(1.0_f / 2.2_f));

            fragColor = envColor;
            fragID = -1_u;
            fragNormalDepth = vec4(0.0_f);
            fragPos = fragPosition;
        });

        std::vector<uint32_t> bytecode =
            spirv::serialiseSpirv(writer.getShader());

        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
        createInfo.pCode = bytecode.data();

        m_fragmentModule = vk.create(createInfo);
        if (!m_fragmentModule)
        {
            std::cerr << "error: failed to create fragment shader module"
                      << std::endl;
            abort();
        }
    }
#pragma endregion

#pragma region vertex buffer
    std::vector<vector3> vertices{
        // back face
        vector3{ -1.0f, -1.0f, -1.0f },
        vector3{ 1.0f, 1.0f, -1.0f },
        vector3{ 1.0f, -1.0f, -1.0f },
        vector3{ 1.0f, 1.0f, -1.0f },
        vector3{ -1.0f, -1.0f, -1.0f },
        vector3{ -1.0f, 1.0f, -1.0f },
        // front face
        vector3{ -1.0f, -1.0f, 1.0f },
        vector3{ 1.0f, -1.0f, 1.0f },
        vector3{ 1.0f, 1.0f, 1.0f },
        vector3{ 1.0f, 1.0f, 1.0f },
        vector3{ -1.0f, 1.0f, 1.0f },
        vector3{ -1.0f, -1.0f, 1.0f },
        // left face
        vector3{ -1.0f, 1.0f, 1.0f },
        vector3{ -1.0f, 1.0f, -1.0f },
        vector3{ -1.0f, -1.0f, -1.0f },
        vector3{ -1.0f, -1.0f, -1.0f },
        vector3{ -1.0f, -1.0f, 1.0f },
        vector3{ -1.0f, 1.0f, 1.0f },
        // right face
        vector3{ 1.0f, 1.0f, 1.0f },
        vector3{ 1.0f, -1.0f, -1.0f },
        vector3{ 1.0f, 1.0f, -1.0f },
        vector3{ 1.0f, -1.0f, -1.0f },
        vector3{ 1.0f, 1.0f, 1.0f },
        vector3{ 1.0f, -1.0f, 1.0f },
        // bottom face
        vector3{ -1.0f, -1.0f, -1.0f },
        vector3{ 1.0f, -1.0f, -1.0f },
        vector3{ 1.0f, -1.0f, 1.0f },
        vector3{ 1.0f, -1.0f, 1.0f },
        vector3{ -1.0f, -1.0f, 1.0f },
        vector3{ -1.0f, -1.0f, -1.0f },
        // top face
        vector3{ -1.0f, 1.0f, -1.0f },
        vector3{ 1.0f, 1.0f, 1.0f },
        vector3{ 1.0f, 1.0f, -1.0f },
        vector3{ 1.0f, 1.0f, 1.0f },
        vector3{ -1.0f, 1.0f, -1.0f },
        vector3{ -1.0f, 1.0f, 1.0f },
    };

    m_vertexBuffer = Buffer(
        vk, vertices.size() * sizeof(*vertices.data()),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    StagingBuffer verticesStagingBuffer(vk, vertices.data(), vertices.size());

    CommandBuffer copyCB(vk, rw.get().oneTimeCommandPool());
    copyCB.begin();
    copyCB.copyBuffer(verticesStagingBuffer, m_vertexBuffer,
                      sizeof(*vertices.data()) * vertices.size());
    copyCB.end();

    if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
        throw std::runtime_error("could not copy vertex buffer");
    vk.wait(vk.graphicsQueue());
#pragma endregion

#pragma region descriptor pool
    std::array poolSizes{
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
    };

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = uint32_t(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    m_descriptorPool = vk.create(poolInfo);
    if (!m_descriptorPool)
    {
        std::cerr << "error: failed to create descriptor pool" << std::endl;
        abort();
    }
#pragma endregion

#pragma region descriptor set layout
    VkDescriptorSetLayoutBinding layoutBindingUbo{};
    layoutBindingUbo.binding = 0;
    layoutBindingUbo.descriptorCount = 1;
    layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindingUbo.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding layoutBindingCubemap{};
    layoutBindingCubemap.binding = 1;
    layoutBindingCubemap.descriptorCount = 1;
    layoutBindingCubemap.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindingCubemap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array layoutBindings{
        layoutBindingUbo,
        layoutBindingCubemap,
    };

    vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
    setLayoutInfo.bindingCount = uint32_t(layoutBindings.size());
    setLayoutInfo.pBindings = layoutBindings.data();

    m_descriptorSetLayout = vk.create(setLayoutInfo);
    if (!m_descriptorSetLayout)
    {
        std::cerr << "error: failed to create descriptor set layout"
                  << std::endl;
        abort();
    }
#pragma endregion

#pragma region descriptor set
    m_descriptorSet = vk.allocate(m_descriptorPool, m_descriptorSetLayout);

    if (!m_descriptorSet)
    {
        std::cerr << "error: failed to allocate descriptor set" << std::endl;
        abort();
    }
#pragma endregion

#pragma region pipeline layout
    // VkPushConstantRange pcRange{};
    // pcRange.size = sizeof(Mesh::MaterialData);
    // pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout.get();
    // pipelineLayoutInfo.setLayoutCount = 0;
    // pipelineLayoutInfo.pSetLayouts = nullptr;
    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    // pipelineLayoutInfo.pPushConstantRanges = &pcRange;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    m_pipelineLayout = vk.create(pipelineLayoutInfo);
    if (!m_pipelineLayout)
    {
        std::cerr << "error: failed to create pipeline layout" << std::endl;
        abort();
    }
#pragma endregion

#pragma region pipeline
    std::cout << "pipeline" << std::endl;
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertexModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragmentModule;
    fragShaderStageInfo.pName = "main";

    std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    VkVertexInputBindingDescription binding = {};
    binding.binding = 0;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding.stride = sizeof(vector3);

    VkVertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.location = 0;
    positionAttribute.offset = 0;

    std::array attributes = {
        positionAttribute,
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount =
        uint32_t(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = false;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent.width = uint32_t(m_viewport.width);
    scissor.extent.height = uint32_t(m_viewport.height);

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = false;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = false;
    multisampling.alphaToOneEnable = false;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = false;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // colorBlendAttachment.blendEnable = true;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor =
    //    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendAttachmentState objectIDBlendAttachment = {};
    objectIDBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    objectIDBlendAttachment.blendEnable = false;
    objectIDBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    objectIDBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    objectIDBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    objectIDBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    objectIDBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    objectIDBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendAttachmentState normalDepthBlendAttachment = {};
    normalDepthBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    normalDepthBlendAttachment.blendEnable = false;
    normalDepthBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    normalDepthBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    normalDepthBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    normalDepthBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    normalDepthBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    normalDepthBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendAttachmentState positionBlendAttachment = {};
    positionBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    positionBlendAttachment.blendEnable = false;
    positionBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    positionBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    positionBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    positionBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    positionBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    positionBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // colorHDRBlendAttachment.blendEnable = true;
    // colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorHDRBlendAttachment.dstColorBlendFactor =
    //    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    std::array colorBlendAttachments{ colorBlendAttachment,
                                      objectIDBlendAttachment,
                                      normalDepthBlendAttachment,
                                      positionBlendAttachment };

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = false;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = uint32_t(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = false;
    depthStencil.minDepthBounds = 0.0f;  // Optional
    depthStencil.maxDepthBounds = 1.0f;  // Optional
    depthStencil.stencilTestEnable = false;
    // depthStencil.front; // Optional
    // depthStencil.back; // Optional

    std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                 VK_DYNAMIC_STATE_LINE_WIDTH };

    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount = uint32_t(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = uint32_t(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    m_pipeline = vk.create(pipelineInfo);
    if (!m_pipeline)
    {
        std::cerr << "error: failed to create graphics pipeline" << std::endl;
        abort();
    }
#pragma endregion

#pragma region UBO
    m_ubo = Buffer(vk, sizeof(m_config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VMA_MEMORY_USAGE_CPU_TO_GPU,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    m_config.view = matrix4::identity();
    m_config.proj = matrix4::identity();

    m_config.copyTo(m_ubo.map());
    m_ubo.unmap();

    VkDescriptorBufferInfo setBufferInfo{};
    setBufferInfo.buffer = m_ubo;
    setBufferInfo.range = sizeof(m_config);
    setBufferInfo.offset = 0;

    vk::WriteDescriptorSet uboWrite;
    uboWrite.descriptorCount = 1;
    uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboWrite.dstArrayElement = 0;
    uboWrite.dstBinding = 0;
    uboWrite.dstSet = m_descriptorSet;
    uboWrite.pBufferInfo = &setBufferInfo;

    vk.updateDescriptorSets(uboWrite);
#pragma endregion

    //#pragma region equirectangularHDR
    //  int w, h, c;
    //  float* imageData = stbi_loadf(filepath.data(), &w, &h, &c, 4);
    //
    //  if (!imageData)
    //      throw std::runtime_error("could not load equirectangular map");
    //
    //  m_equirectangularTexture = Texture2D(
    //      rw, w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
    //      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    //      VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //
    //  VkBufferImageCopy copy{};
    //  copy.bufferRowLength = w;
    //  copy.bufferImageHeight = h;
    //  copy.imageExtent.width = w;
    //  copy.imageExtent.height = h;
    //  copy.imageExtent.depth = 1;
    //  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //  copy.imageSubresource.baseArrayLayer = 0;
    //  copy.imageSubresource.layerCount = 1;
    //  copy.imageSubresource.mipLevel = 0;
    //
    //  m_equirectangularTexture.uploadDataImmediate(
    //      imageData, w * h * 4 * sizeof(float), copy,
    // VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //
    //  stbi_image_free(imageData);
    //#pragma endregion

#pragma region cubemaps
    {
        // EquirectangularToCubemap e2c(rw, 2048);
        // m_cubemap = e2c.computeCubemap(m_equirectangularTexture);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_cubemap.view();
        imageInfo.sampler = m_cubemap.sampler();

        vk::WriteDescriptorSet textureWrite;
        textureWrite.descriptorCount = 1;
        textureWrite.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite.dstArrayElement = 0;
        textureWrite.dstBinding = 1;
        textureWrite.dstSet = m_descriptorSet;
        textureWrite.pImageInfo = &imageInfo;

        vk.updateDescriptorSets({ textureWrite });
    }
#pragma endregion
}

Skybox::~Skybox() {}

void Skybox::setMatrices(matrix4 projection, matrix4 view)
{
    m_config.view = view;
    m_config.proj = projection;

    m_config.copyTo(m_ubo.map());
    m_ubo.unmap();
}

void Skybox::render(CommandBuffer& cb)
{
    cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0,
                         m_descriptorSet);

    cb.bindVertexBuffer(m_vertexBuffer);
    cb.draw(36);
}
}  // namespace cdm
