#pragma once

#include "core/Logger/Logger.h"
#include "core/Window/Window.h"
#include "defines.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<u32> GraphicsFamily;
    std::optional<u32> PresentFamily;

     bool IsComplete() {
        return GraphicsFamily.has_value() && PresentFamily.has_value();
    }
};

struct VulkanDevice
{
    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
    VkPhysicalDeviceProperties Properties;
    VkPhysicalDeviceFeatures Features;
};

struct VulkanContext {
    VkInstance Instance;
    VkAllocationCallbacks* Allocator;
    VulkanDevice VulkanDevice;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkSurfaceKHR Surface;
    VkSwapchainKHR SwapChain;
    VkDebugUtilsMessengerEXT DebugMessenger;
    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImageViews;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    VkRenderPass RenderPass;
    VkDescriptorSetLayout DescriptorSetLayout;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;
    std::vector<VkFramebuffer> Framebuffers;
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> InFlightFences;
    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;
    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void*> UniformBuffersMapped;
    VkDescriptorPool DescriptoPool;
    std::vector<VkDescriptorSet> DescriptorSets;
};

struct SwapChainSupport
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#if defined(_DEBUG) 
    const bool EnableValidationLayers = false;
#else
    const bool EnableValidationLayers = true;
#endif