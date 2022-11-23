#include "Renderer.h"
#include "core/Logger/Logger.h"
#include "core/Window/Window.h"
#include "defines.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <vulkan/vulkan_win32.h>
#include <algorithm>
#include <set>
#define GLFW_INCLUDE_VULKAN
#include <vendor/GLFW/glfw3.h>

static VulkanContext VulkanContext;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

Renderer::Renderer() {
}

Renderer::~Renderer() {
}

bool Renderer::Initialize(const char* name, Window* window) {
    
    if (EnableValidationLayers && !CheckValidationLayerSupport()) {
        EM_FATAL("Validation layers requested, but not available");
        return false;
    }

    bool instance = CreateVulkanInstance();
    if (!EnableValidationLayers) {
        CreateDebugger();
    }
    bool surface = CreateVulkanSurface(&window->State);
    bool hasDevice = PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain(&window->State);

    return instance && surface && hasDevice;
}

bool Renderer::CreateVulkanInstance() {
    VulkanContext.Allocator = 0;
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.pApplicationName = "Splintered";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Splintered Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    std::vector<const char*> enabledInstanceExtensions;
    enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    enabledInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

#if defined(_DEBUG)
    enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (u32)enabledInstanceExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
    createInfo.enabledLayerCount = static_cast<u32>(ValidationLayers.size());
    createInfo.ppEnabledLayerNames = ValidationLayers.data();

    VkResult result = vkCreateInstance(&createInfo, VulkanContext.Allocator, &VulkanContext.Instance);

    if (result != VK_SUCCESS) {
        EM_FATAL("vkCreateInstance failed with result");
        return false;
    }

    EM_INFO("Vulkan Instance initialized successfully.");

    return true;
}

bool Renderer::CreateVulkanSurface(WindowState* state) {
    
    VkWin32SurfaceCreateInfoKHR surface_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surface_info.pNext = NULL;
    surface_info.hinstance = state->GetWindow();
    surface_info.hwnd = state->GetHWND();

    VkResult result = vkCreateWin32SurfaceKHR(VulkanContext.Instance, &surface_info, NULL, &state->Surface);

    if (result != VK_SUCCESS) {
        EM_FATAL("Vulkan surface creation failed. %e", result);
        return false;
    }

    VulkanContext.Surface = state->Surface;

    EM_INFO("Vulkan Surface initialized successfully.");

    return true;
}

bool Renderer::PickPhysicalDevice() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(VulkanContext.Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        EM_FATAL("No devices compatible with vulkan");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(VulkanContext.Instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceCompatible(device)) {
            break;
        }
    }

    if (VulkanContext.VulkanDevice.PhysicalDevice == VK_NULL_HANDLE) {
        EM_FATAL("Null vulkan device");
        return false;
    }

    EM_INFO("Using device %s", VulkanContext.VulkanDevice.Properties.deviceName);

    return true;
}

void Renderer::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(VulkanContext.VulkanDevice.PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &VulkanContext.VulkanDevice.Features;
    createInfo.enabledExtensionCount = static_cast<u32>(DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

    if (EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        createInfo.ppEnabledLayerNames = ValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(VulkanContext.VulkanDevice.PhysicalDevice, &createInfo, nullptr, &VulkanContext.VulkanDevice.LogicalDevice) != VK_SUCCESS) {
        EM_FATAL("Failed to create logical device!");
    }

    EM_INFO("Logical device created successfully!");

    vkGetDeviceQueue(VulkanContext.VulkanDevice.LogicalDevice, indices.GraphicsFamily.value(), 0, &VulkanContext.GraphicsQueue);
    vkGetDeviceQueue(VulkanContext.VulkanDevice.LogicalDevice, indices.PresentFamily.value(), 0, &VulkanContext.PresentQueue);
}

void Renderer::CreateSwapChain(WindowState* state) {

    SwapChainSupport swapChainSupport = QuerySwapChainSupport(VulkanContext.VulkanDevice.PhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities, state);

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (swapChainSupport.Capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		pre_transform = swapChainSupport.Capabilities.currentTransform;
	}


    u32 imageCount = swapChainSupport.Capabilities.minImageCount + 1;
    if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount) {
        imageCount = swapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface = VulkanContext.Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = pre_transform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VulkanContext.SwapChain;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateSwapchainKHR(VulkanContext.VulkanDevice.LogicalDevice, &createInfo, nullptr, &VulkanContext.SwapChain);

    if (result != VK_SUCCESS)
    {
        EM_FATAL("Could not create surface");
    }

    vkGetSwapchainImagesKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, &imageCount, nullptr);
    VulkanContext.SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, &imageCount, VulkanContext.SwapChainImages.data());

    VulkanContext.SwapChainImageFormat = surfaceFormat.format;
    VulkanContext.SwapChainExtent = extent;
}

bool Renderer::IsDeviceCompatible(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = FindQueueFamilies(device);

    if (indices.IsComplete()) {
        VulkanContext.VulkanDevice.Features = deviceFeatures;
        VulkanContext.VulkanDevice.Properties = deviceProperties;
        VulkanContext.VulkanDevice.PhysicalDevice = device;
    }

    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupport swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
    }

    return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}

bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    u32 extCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);

    std::vector<VkExtensionProperties> availableDeviceExtension(extCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, availableDeviceExtension.data());

    std::set<std::string> reqExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

    for (const auto& extension : availableDeviceExtension) {
        reqExtensions.erase(extension.extensionName);
    }

    return reqExtensions.empty();
}

QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VulkanContext.Surface, &presentSupport);
            if (presentSupport) {
                indices.PresentFamily = i;
            }
            indices.GraphicsFamily = i;
        }

        if (indices.IsComplete())
            break;

        i++;
    }

    return indices;
}

bool Renderer::CheckValidationLayerSupport() {
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : ValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        return layerFound;
    }

    return false;
}

void Renderer::CreateDebugger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VulkanDebugCallback;
    createInfo.pUserData = nullptr;

    VkResult result = CreateDebugUtilsMessengerEXT(VulkanContext.Instance, &createInfo, nullptr, &VulkanContext.DebugMessenger);

    if (result != VK_SUCCESS) {
        EM_ERROR("Could not create vulkan debug messenger.");
    } else {
        EM_INFO("Debugger attached");
    }
}

SwapChainSupport Renderer::QuerySwapChainSupport(VkPhysicalDevice device) {
    
    SwapChainSupport details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VulkanContext.Surface, &details.Capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanContext.Surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanContext.Surface, &formatCount, details.Formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanContext.Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanContext.Surface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, WindowState* state) {

  if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return surfaceCapabilities.currentExtent;
    } else {
        
        int width, height;
        glfwGetFramebufferSize(state->GlfwWindow, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkDevice Renderer::GetLogicalDevice()
{
    return VulkanContext.VulkanDevice.LogicalDevice;
}

void Renderer::Shutdown() {
    vkDestroySwapchainKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, nullptr);

    if (EnableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(VulkanContext.Instance, VulkanContext.DebugMessenger, nullptr);
    }

    vkDestroyDevice(VulkanContext.VulkanDevice.LogicalDevice, nullptr);
    vkDestroySurfaceKHR(VulkanContext.Instance, VulkanContext.Surface, nullptr);
    vkDestroyInstance(VulkanContext.Instance, nullptr);
}