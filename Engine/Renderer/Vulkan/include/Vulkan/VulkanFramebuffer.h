#pragma once

#include <vulkan/vulkan.hpp>

struct FrameBufferSpec
{
};

class VulkanFramebuffer
{
public:
    VulkanFramebuffer(uint32_t width, uint32_t height);

private:
    vk::UniqueFramebuffer framebuffer;

    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView imageView;
};
