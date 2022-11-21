#include "Renderer.h"
#include "core/Logger/Logger.h"
#include "core/Window/Window.h"
#include "defines.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <vulkan/vulkan_win32.h>

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
    bool surface = CreateVulkanSurface(window->State);

    if (EnableValidationLayers) {
        CreateDebugger();
    }

    bool hasDevice = PickPhysicalDevice();

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
    surface_info.hinstance = state->HInstance;
    surface_info.hwnd = state->Hwnd;

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

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.GraphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;

    createInfo.pEnabledFeatures = &VulkanContext.VulkanDevice.Features;

    createInfo.enabledExtensionCount = 0;

    if (EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        createInfo.ppEnabledLayerNames = ValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(VulkanContext.VulkanDevice.PhysicalDevice, &createInfo, nullptr, &VulkanContext.VulkanDevice.LogicalDevice) != VK_SUCCESS) {
        EM_FATAL("Failed to create logical device!");
    }

    vkGetDeviceQueue(VulkanContext.VulkanDevice.LogicalDevice, indices.GraphicsFamily.value(), 0, &VulkanContext.GraphicsQueue);
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
        return true;
    }

    return false;
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

void Renderer::Shutdown() {
    if (EnableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(VulkanContext.Instance, VulkanContext.DebugMessenger, nullptr);
    }

     vkDestroyDevice(VulkanContext.VulkanDevice.LogicalDevice, nullptr);

    vkDestroyInstance(VulkanContext.Instance, nullptr);
}