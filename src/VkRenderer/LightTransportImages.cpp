#include "LightTransport.hpp"
#include "cdm_maths.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

constexpr size_t POINT_COUNT = 2000;

#define TEX_SCALE 1

constexpr uint32_t width = 1280 * TEX_SCALE;
constexpr uint32_t height = 720 * TEX_SCALE;
constexpr float widthf = 1280.0f * TEX_SCALE;
constexpr float heightf = 720.0f * TEX_SCALE;

namespace cdm
{
void LightTransport::createImages()
{
    auto& vk = rw.get().device();

#pragma region outputImage
    m_outputImage = Texture2D(
        rw, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk.debugMarkerSetObjectName(m_outputImage.get(), "outputImage");

    m_outputImage.transitionLayoutImmediate(VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region outputImageHDR
    m_outputImageHDR = Texture2D(
        rw, width, height, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk.debugMarkerSetObjectName(m_outputImageHDR.get(), "outputImageHDR");

    m_outputImageHDR.transitionLayoutImmediate(VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_GENERAL);
#pragma endregion

    /*
#pragma region waveLengthTransferFunction
    // clang-format off
    std::vector<vector4> tf = {
        vector4(+0.00045, -0.00048, +0.00344, 0.0),
        vector4(+0.00054, -0.00057, +0.00411, 0.0),
        vector4(+0.00064, -0.00068, +0.00491, 0.0),
        vector4(+0.00076, -0.00081, +0.00587, 0.0),
        vector4(+0.00091, -0.00097, +0.00702, 0.0),
        vector4(+0.00109, -0.00116, +0.00840, 0.0),
        vector4(+0.00129, -0.00138, +0.01001, 0.0),
        vector4(+0.00153, -0.00164, +0.01190, 0.0),
        vector4(+0.00182, -0.00194, +0.01414, 0.0),
        vector4(+0.00218, -0.00234, +0.01703, 0.0),
        vector4(+0.00264, -0.00284, +0.02071, 0.0),
        vector4(+0.00322, -0.00348, +0.02534, 0.0),
        vector4(+0.00386, -0.00417, +0.03038, 0.0),
        vector4(+0.00450, -0.00486, +0.03546, 0.0),
        vector4(+0.00516, -0.00558, +0.04072, 0.0),
        vector4(+0.00598, -0.00647, +0.04724, 0.0),
        vector4(+0.00710, -0.00769, +0.05613, 0.0),
        vector4(+0.00863, -0.00936, +0.06836, 0.0),
        vector4(+0.01053, -0.01144, +0.08357, 0.0),
        vector4(+0.01274, -0.01385, +0.10125, 0.0),
        vector4(+0.01519, -0.01653, +0.12093, 0.0),
        vector4(+0.01803, -0.01965, +0.14389, 0.0),
        vector4(+0.02168, -0.02365, +0.17330, 0.0),
        vector4(+0.02643, -0.02888, +0.21162, 0.0),
        vector4(+0.03245, -0.03552, +0.26028, 0.0),
        vector4(+0.03908, -0.04284, +0.31394, 0.0),
        vector4(+0.04569, -0.05016, +0.36771, 0.0),
        vector4(+0.05227, -0.05746, +0.42149, 0.0),
        vector4(+0.06027, -0.06638, +0.48734, 0.0),
        vector4(+0.07081, -0.07815, +0.57443, 0.0),
        vector4(+0.08552, -0.09461, +0.69639, 0.0),
        vector4(+0.10362, -0.11491, +0.84703, 0.0),
        vector4(+0.12528, -0.13927, +1.02788, 0.0),
        vector4(+0.14983, -0.16697, +1.23378, 0.0),
        vector4(+0.17827, -0.19920, +1.47406, 0.0),
        vector4(+0.21158, -0.23708, +1.75729, 0.0),
        vector4(+0.25101, -0.28203, +2.09453, 0.0),
        vector4(+0.29624, -0.33380, +2.48475, 0.0),
        vector4(+0.34779, -0.39321, +2.93667, 0.0),
        vector4(+0.40658, -0.46162, +3.46319, 0.0),
        vector4(+0.47220, -0.53867, +4.06305, 0.0),
        vector4(+0.54051, -0.61973, +4.70338, 0.0),
        vector4(+0.60635, -0.69886, +5.33983, 0.0),
        vector4(+0.66537, -0.77121, +5.93862, 0.0),
        vector4(+0.71768, -0.83688, +6.49979, 0.0),
        vector4(+0.76113, -0.89376, +7.01109, 0.0),
        vector4(+0.79616, -0.94228, +7.47436, 0.0),
        vector4(+0.82035, -0.97986, +7.87338, 0.0),
        vector4(+0.83614, -1.00873, +8.22049, 0.0),
        vector4(+0.84242, -1.02744, +8.50645, 0.0),
        vector4(+0.84078, -1.03762, +8.74118, 0.0),
        vector4(+0.83034, -1.03859, +8.92043, 0.0),
        vector4(+0.81254, -1.03214, +9.05476, 0.0),
        vector4(+0.78703, -1.01809, +9.14316, 0.0),
        vector4(+0.75484, -0.99759, +9.19545, 0.0),
        vector4(+0.71615, -0.97105, +9.21808, 0.0),
        vector4(+0.67160, -0.93921, +9.21831, 0.0),
        vector4(+0.62161, -0.90259, +9.20002, 0.0),
        vector4(+0.56689, -0.86205, +9.17044, 0.0),
        vector4(+0.50834, -0.81861, +9.13661, 0.0),
        vector4(+0.44594, -0.77208, +9.09795, 0.0),
        vector4(+0.37943, -0.72170, +9.04709, 0.0),
        vector4(+0.30755, -0.66573, +8.97282, 0.0),
        vector4(+0.23092, -0.60438, +8.87416, 0.0),
        vector4(+0.14981, -0.53723, +8.74625, 0.0),
        vector4(+0.06594, -0.46565, +8.59299, 0.0),
        vector4(-0.02033, -0.38919, +8.40964, 0.0),
        vector4(-0.10876, -0.30770, +8.19449, 0.0),
        vector4(-0.20050, -0.21796, +7.92484, 0.0),
        vector4(-0.29554, -0.11954, +7.59637, 0.0),
        vector4(-0.39331, -0.01241, +7.21152, 0.0),
        vector4(-0.49150, +0.09945, +6.80323, 0.0),
        vector4(-0.58821, +0.21359, +6.39517, 0.0),
        vector4(-0.68287, +0.32900, +5.99509, 0.0),
        vector4(-0.77548, +0.44604, +5.60104, 0.0),
        vector4(-0.86610, +0.56489, +5.21161, 0.0),
        vector4(-0.95460, +0.68545, +4.82936, 0.0),
        vector4(-1.04075, +0.80685, +4.45829, 0.0),
        vector4(-1.12455, +0.92810, +4.09979, 0.0),
        vector4(-1.20591, +1.04853, +3.75664, 0.0),
        vector4(-1.28534, +1.16898, +3.42953, 0.0),
        vector4(-1.36289, +1.29080, +3.12382, 0.0),
        vector4(-1.43893, +1.41475, +2.83930, 0.0),
        vector4(-1.51418, +1.54219, +2.57957, 0.0),
        vector4(-1.59057, +1.67393, +2.33844, 0.0),
        vector4(-1.67070, +1.81285, +2.11752, 0.0),
        vector4(-1.75483, +1.95874, +1.91220, 0.0),
        vector4(-1.84334, +2.11293, +1.72443, 0.0),
        vector4(-1.93521, +2.27405, +1.55044, 0.0),
        vector4(-2.03042, +2.44286, +1.39069, 0.0),
        vector4(-2.13002, +2.62100, +1.24237, 0.0),
        vector4(-2.23687, +2.81319, +1.10525, 0.0),
        vector4(-2.35236, +3.02163, +0.97760, 0.0),
        vector4(-2.47428, +3.24476, +0.85780, 0.0),
        vector4(-2.59703, +3.47704, +0.74210, 0.0),
        vector4(-2.71594, +3.71384, +0.62737, 0.0),
        vector4(-2.82897, +3.95378, +0.51297, 0.0),
        vector4(-2.93632, +4.19762, +0.39993, 0.0),
        vector4(-3.03724, +4.44667, +0.28956, 0.0),
        vector4(-3.13108, +4.69930, +0.18189, 0.0),
        vector4(-3.21371, +4.95123, +0.07878, 0.0),
        vector4(-3.28397, +5.19852, -0.01863, 0.0),
        vector4(-3.33736, +5.43706, -0.10798, 0.0),
        vector4(-3.37511, +5.66511, -0.19039, 0.0),
        vector4(-3.39057, +5.87377, -0.26368, 0.0),
        vector4(-3.38528, +6.06146, -0.32886, 0.0),
        vector4(-3.35591, +6.22465, -0.38509, 0.0),
        vector4(-3.30812, +6.37058, -0.43551, 0.0),
        vector4(-3.24179, +6.50082, -0.48132, 0.0),
        vector4(-3.15961, +6.61749, -0.52364, 0.0),
        vector4(-3.06176, +6.71875, -0.56240, 0.0),
        vector4(-2.95103, +6.80507, -0.59813, 0.0),
        vector4(-2.82820, +6.87607, -0.63112, 0.0),
        vector4(-2.69357, +6.93282, -0.66147, 0.0),
        vector4(-2.54728, +6.97631, -0.68910, 0.0),
        vector4(-2.38914, +7.00683, -0.71390, 0.0),
        vector4(-2.22005, +7.02525, -0.73614, 0.0),
        vector4(-2.03886, +7.03046, -0.75560, 0.0),
        vector4(-1.84668, +7.02377, -0.77259, 0.0),
        vector4(-1.64223, +7.00396, -0.78687, 0.0),
        vector4(-1.42756, +6.97316, -0.79889, 0.0),
        vector4(-1.20113, +6.92961, -0.80833, 0.0),
        vector4(-0.96551, +6.87626, -0.81577, 0.0),
        vector4(-0.71778, +6.81012, -0.82080, 0.0),
        vector4(-0.46110, +6.73561, -0.82419, 0.0),
        vector4(-0.19336, +6.65064, -0.82569, 0.0),
        vector4(+0.08365, +6.55671, -0.82561, 0.0),
        vector4(+0.37088, +6.45270, -0.82381, 0.0),
        vector4(+0.66727, +6.33938, -0.82051, 0.0),
        vector4(+0.97318, +6.21586, -0.81561, 0.0),
        vector4(+1.28739, +6.08248, -0.80924, 0.0),
        vector4(+1.60955, +5.93863, -0.80137, 0.0),
        vector4(+1.93880, +5.78480, -0.79209, 0.0),
        vector4(+2.27437, +5.62195, -0.78153, 0.0),
        vector4(+2.61591, +5.45037, -0.76973, 0.0),
        vector4(+2.96233, +5.27108, -0.75682, 0.0),
        vector4(+3.31285, +5.08373, -0.74275, 0.0),
        vector4(+3.66602, +4.88945, -0.72766, 0.0),
        vector4(+4.02090, +4.68771, -0.71146, 0.0),
        vector4(+4.37621, +4.48023, -0.69439, 0.0),
        vector4(+4.73064, +4.26687, -0.67639, 0.0),
        vector4(+5.08331, +4.04937, -0.65773, 0.0),
        vector4(+5.43209, +3.82766, -0.63835, 0.0),
        vector4(+5.77578, +3.60300, -0.61845, 0.0),
        vector4(+6.11136, +3.37554, -0.59802, 0.0),
        vector4(+6.43833, +3.14646, -0.57722, 0.0),
        vector4(+6.75425, +2.91678, -0.55610, 0.0),
        vector4(+7.05863, +2.68778, -0.53481, 0.0),
        vector4(+7.34897, +2.46063, -0.51343, 0.0),
        vector4(+7.62475, +2.23621, -0.49201, 0.0),
        vector4(+7.88502, +2.01551, -0.47056, 0.0),
        vector4(+8.12917, +1.79937, -0.44912, 0.0),
        vector4(+8.35493, +1.58891, -0.42780, 0.0),
        vector4(+8.55610, +1.38639, -0.40686, 0.0),
        vector4(+8.72994, +1.19299, -0.38646, 0.0),
        vector4(+8.87308, +1.01015, -0.36670, 0.0),
        vector4(+8.99180, +0.83597, -0.34746, 0.0),
        vector4(+9.08621, +0.67076, -0.32872, 0.0),
        vector4(+9.16062, +0.51316, -0.31041, 0.0),
        vector4(+9.20686, +0.36631, -0.29275, 0.0),
        vector4(+9.22610, +0.22973, -0.27574, 0.0),
        vector4(+9.21024, +0.10654, -0.25962, 0.0),
        vector4(+9.16717, -0.00588, -0.24419, 0.0),
        vector4(+9.09596, -0.10655, -0.22948, 0.0),
        vector4(+9.00259, -0.19742, -0.21535, 0.0),
        vector4(+8.88469, -0.27749, -0.20186, 0.0),
        vector4(+8.74432, -0.34784, -0.18898, 0.0),
        vector4(+8.58049, -0.40808, -0.17677, 0.0),
        vector4(+8.39510, -0.45899, -0.16519, 0.0),
        vector4(+8.18703, -0.50078, -0.15418, 0.0),
        vector4(+7.95487, -0.53377, -0.14371, 0.0),
        vector4(+7.70010, -0.55866, -0.13371, 0.0),
        vector4(+7.42649, -0.57586, -0.12424, 0.0),
        vector4(+7.14029, -0.58670, -0.11524, 0.0),
        vector4(+6.84421, -0.59128, -0.10677, 0.0),
        vector4(+6.54303, -0.59108, -0.09875, 0.0),
        vector4(+6.24334, -0.58674, -0.09130, 0.0),
        vector4(+5.95045, -0.57990, -0.08435, 0.0),
        vector4(+5.66682, -0.57050, -0.07797, 0.0),
        vector4(+5.38831, -0.55895, -0.07199, 0.0),
        vector4(+5.11322, -0.54477, -0.06645, 0.0),
        vector4(+4.84120, -0.52865, -0.06124, 0.0),
        vector4(+4.57352, -0.51058, -0.05640, 0.0),
        vector4(+4.30991, -0.49103, -0.05187, 0.0),
        vector4(+4.05110, -0.47010, -0.04764, 0.0),
        vector4(+3.79769, -0.44817, -0.04369, 0.0),
        vector4(+3.55123, -0.42545, -0.04004, 0.0),
        vector4(+3.31243, -0.40220, -0.03666, 0.0),
        vector4(+3.08214, -0.37871, -0.03354, 0.0),
        vector4(+2.86197, -0.35542, -0.03067, 0.0),
        vector4(+2.65285, -0.33270, -0.02800, 0.0),
        vector4(+2.45568, -0.31075, -0.02556, 0.0),
        vector4(+2.26865, -0.28947, -0.02330, 0.0),
        vector4(+2.09183, -0.26889, -0.02122, 0.0),
        vector4(+1.92374, -0.24898, -0.01930, 0.0),
        vector4(+1.76572, -0.22991, -0.01753, 0.0),
        vector4(+1.61595, -0.21155, -0.01589, 0.0),
        vector4(+1.47600, -0.19411, -0.01440, 0.0),
        vector4(+1.34405, -0.17747, -0.01302, 0.0),
        vector4(+1.22181, -0.16189, -0.01176, 0.0),
        vector4(+1.10773, -0.14727, -0.01060, 0.0),
        vector4(+1.00305, -0.13378, -0.00954, 0.0),
        vector4(+0.90716, -0.12137, -0.00858, 0.0),
        vector4(+0.82139, -0.11020, -0.00773, 0.0),
        vector4(+0.74466, -0.10017, -0.00697, 0.0),
        vector4(+0.67637, -0.09120, -0.00631, 0.0),
        vector4(+0.61518, -0.08315, -0.00571, 0.0),
        vector4(+0.56049, -0.07594, -0.00518, 0.0),
        vector4(+0.51097, -0.06939, -0.00470, 0.0),
        vector4(+0.46462, -0.06325, -0.00425, 0.0),
        vector4(+0.42008, -0.05731, -0.00383, 0.0),
        vector4(+0.37742, -0.05161, -0.00343, 0.0),
        vector4(+0.33796, -0.04630, -0.00305, 0.0),
        vector4(+0.30196, -0.04144, -0.00272, 0.0),
        vector4(+0.26980, -0.03708, -0.00242, 0.0),
        vector4(+0.24065, -0.03310, -0.00216, 0.0),
        vector4(+0.21505, -0.02960, -0.00193, 0.0),
        vector4(+0.19221, -0.02648, -0.00172, 0.0),
        vector4(+0.17238, -0.02376, -0.00154, 0.0),
        vector4(+0.15483, -0.02136, -0.00138, 0.0),
        vector4(+0.13963, -0.01927, -0.00124, 0.0),
        vector4(+0.12619, -0.01742, -0.00112, 0.0),
        vector4(+0.11421, -0.01577, -0.00102, 0.0),
        vector4(+0.10320, -0.01425, -0.00092, 0.0),
        vector4(+0.09311, -0.01286, -0.00083, 0.0),
        vector4(+0.08395, -0.01159, -0.00075, 0.0),
        vector4(+0.07572, -0.01046, -0.00067, 0.0),
        vector4(+0.06834, -0.00944, -0.00061, 0.0),
        vector4(+0.06165, -0.00851, -0.00055, 0.0),
        vector4(+0.05561, -0.00768, -0.00049, 0.0),
        vector4(+0.05009, -0.00692, -0.00045, 0.0),
        vector4(+0.04513, -0.00623, -0.00040, 0.0),
        vector4(+0.04059, -0.00560, -0.00036, 0.0),
        vector4(+0.03653, -0.00504, -0.00033, 0.0),
        vector4(+0.03281, -0.00453, -0.00029, 0.0),
        vector4(+0.02951, -0.00407, -0.00026, 0.0),
        vector4(+0.02652, -0.00366, -0.00024, 0.0),
        vector4(+0.02386, -0.00330, -0.00021, 0.0),
        vector4(+0.02146, -0.00296, -0.00019, 0.0),
        vector4(+0.01930, -0.00267, -0.00017, 0.0),
        vector4(+0.01734, -0.00239, -0.00015, 0.0),
        vector4(+0.01557, -0.00215, -0.00014, 0.0),
        vector4(+0.01396, -0.00193, -0.00012, 0.0),
        vector4(+0.01250, -0.00173, -0.00011, 0.0),
        vector4(+0.01118, -0.00154, -0.00010, 0.0),
        vector4(+0.01000, -0.00138, -0.00009, 0.0),
        vector4(+0.00893, -0.00123, -0.00008, 0.0),
        vector4(+0.00797, -0.00110, -0.00007, 0.0),
        vector4(+0.00712, -0.00098, -0.00006, 0.0),
        vector4(+0.00635, -0.00088, -0.00006, 0.0),
        vector4(+0.00567, -0.00078, -0.00005, 0.0),
        vector4(+0.00506, -0.00070, -0.00005, 0.0),
        vector4(+0.00453, -0.00062, -0.00004, 0.0),
        vector4(+0.00405, -0.00056, -0.00004, 0.0),
        vector4(+0.00363, -0.00050, -0.00003, 0.0),
        vector4(+0.00326, -0.00045, -0.00003, 0.0)
    };
    // clang-format on

    m_waveLengthTransferFunction = Texture1D(
        rw, tf.size(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk.debugMarkerSetObjectName(m_waveLengthTransferFunction.get(),
                                "waveLengthTransferFunction");

    VkBufferImageCopy region{};
    region.bufferRowLength = tf.size();
    region.bufferImageHeight = 1;
    region.imageExtent = m_waveLengthTransferFunction.extent3D();
    // region.imageExtent.width = tf.size();
    // region.imageExtent.height = 1;
    // region.imageExtent.depth = 1;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = 0;

    m_waveLengthTransferFunction.uploadDataImmediate(
        tf.data(), sizeof(*tf.data()) * tf.size(), region,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    m_waveLengthTransferFunction.transitionLayoutImmediate(
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
#pragma endregion
    //*/
}
}  // namespace cdm
