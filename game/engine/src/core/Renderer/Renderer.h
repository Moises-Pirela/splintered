#pragma once

#include "VulkanTypes.h"
#include "core/Window/Window.h"
#include "defines.h"
#include <vulkan/vulkan.h>
#include <vector>

class Renderer {
public:
    u32 CurrentFrame = 0;
    bool FramebufferResized = false;
    Window* MainWindow;
    
    public:
    Renderer();
    ~Renderer();

    bool Initialize(const char* appName, Window* window);
    void Draw();
    void Shutdown();
    VkDevice GetLogicalDevice();

    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
    {
        switch (messageSeverity) {
            case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                EM_INFO("Validation Layer: %s", pCallbackData->pMessage);
                break;
            case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                EM_INFO("Validation Layer: %s", pCallbackData->pMessage);
                break;
            case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                EM_WARN("Validation Layer: %s", pCallbackData->pMessage);
                break;
            case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                EM_ERROR("Validation Layer: %s", pCallbackData->pMessage);
                break;
            case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
                EM_INFO("Validation Layer: %s", pCallbackData->pMessage);
                break;
        }
        return VK_FALSE;
    }

private:
    bool CreateVulkanInstance();
    bool CreateVulkanSurface(WindowState* state);
    bool CheckValidationLayerSupport();
    void CreateDebugger();
    bool PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain(WindowState* state);
    void RecreateSwapChain();
    void CleanSwapChain();
    bool IsDeviceCompatible(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    SwapChainSupport QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, WindowState* state);
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateFrameBuffers();
    void CreateCommandPool();
    void CreateCommandBuffer();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);
    void CreateSyncObjects();
    void CreateUniformBuffers();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CreateDescriptorSetLayout();
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void UpdateUniformBuffer(u32 currentImage);
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    u32  FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
};
