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
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
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

class Renderer {
    public:
    Renderer();
    ~Renderer();

    bool Initialize(const char* appName, Window* window);
    bool CreateVulkanInstance();
    bool CreateVulkanSurface(WindowState* state);
    bool CheckValidationLayerSupport();
    void CreateDebugger();
    bool PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain(WindowState* state);
    bool IsDeviceCompatible(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    SwapChainSupport QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, WindowState* state);
    VkDevice GetLogicalDevice();
    void Shutdown();

    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
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
};
