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

     bool IsComplete() {

        //EM_DEBUG("u32 %d : int %d", GraphicsFamily, (int)GraphicsFamily);
       // EM_DEBUG("familiy %d", &GraphicsFamily.value);

        return GraphicsFamily.has_value();
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
    VkSurfaceKHR Surface;
    VkDebugUtilsMessengerEXT DebugMessenger;
};

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
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
    bool IsDeviceCompatible(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
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
