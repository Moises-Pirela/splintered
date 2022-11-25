#include "Renderer.h"
#include "core/Logger/Logger.h"
#include "core/Window/Window.h"
#include "defines.h"
#include "VulkanTypes.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <vulkan/vulkan_win32.h>
#include <algorithm>
#include <set>
#include <fstream>
#include <core/Math/Vertex.h>
#include "UniformBuffer.h"
#define GLM_FORCE_RADIANS
#include <vendor/glm/glm/glm.hpp>
#include <vendor/glm/glm/gtc/matrix_transform.hpp>
#include <chrono>
#define GLFW_INCLUDE_VULKAN
#include <vendor/GLFW/glfw3.h>

static VulkanContext VulkanContext;
const int MAX_FRAMES_IN_FLIGHT = 2;

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

static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        EM_FATAL("Could not open file %s", filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->FramebufferResized = true;
}

 static std::vector<Vertex> Vertices = {
   {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> Indices = {
    0, 1, 2, 2, 3, 0
};



Renderer::Renderer() {
}

Renderer::~Renderer() {
}

bool Renderer::Initialize(const char* name, Window* window) {
    MainWindow = window;
    glfwSetWindowUserPointer(window->State.GlfwWindow, this);
    glfwSetFramebufferSizeCallback(window->State.GlfwWindow, FramebufferResizeCallback);

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
    CreateImageViews();
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateFrameBuffers();
    CreateCommandPool();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffer();
    CreateSyncObjects();

    return instance && surface && hasDevice;
}

void Renderer::Draw() {
    vkWaitForFences(VulkanContext.VulkanDevice.LogicalDevice, 1, &VulkanContext.InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, UINT64_MAX, VulkanContext.ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        EM_FATAL("failed to acquire swap chain image!");
    }

    UpdateUniformBuffer(CurrentFrame);

    vkResetFences(VulkanContext.VulkanDevice.LogicalDevice, 1, &VulkanContext.InFlightFences[CurrentFrame]);

    vkResetCommandBuffer(VulkanContext.CommandBuffers[CurrentFrame], 0);

    RecordCommandBuffer(VulkanContext.CommandBuffers[CurrentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {VulkanContext.ImageAvailableSemaphores[CurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &VulkanContext.CommandBuffers[CurrentFrame];

    VkSemaphore signalSemaphores[] = {VulkanContext.RenderFinishedSemaphores[CurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(VulkanContext.GraphicsQueue, 1, &submitInfo, VulkanContext.InFlightFences[CurrentFrame]) != VK_SUCCESS) {
        EM_FATAL("COULD NOT DRAW FRAME!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {VulkanContext.SwapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(VulkanContext.PresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || FramebufferResized) {
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        EM_FATAL("failed to present swap chain image!");
    }

    CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
    if (swapChainSupport.Capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
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
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateSwapchainKHR(VulkanContext.VulkanDevice.LogicalDevice, &createInfo, nullptr, &VulkanContext.SwapChain);

    if (result != VK_SUCCESS) {
        EM_FATAL("Could not create surface");
    }

    vkGetSwapchainImagesKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, &imageCount, nullptr);
    VulkanContext.SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, &imageCount, VulkanContext.SwapChainImages.data());

    VulkanContext.SwapChainImageFormat = surfaceFormat.format;
    VulkanContext.SwapChainExtent = extent;

    EM_INFO("created surface");
}

void Renderer::RecreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(MainWindow->State.GlfwWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(MainWindow->State.GlfwWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(VulkanContext.VulkanDevice.LogicalDevice);

    CleanSwapChain();

    CreateSwapChain(&MainWindow->State);
    CreateImageViews();
    CreateFrameBuffers();
}

void Renderer::CleanSwapChain() {
    for (size_t i = 0; i < VulkanContext.Framebuffers.size(); i++) {
        vkDestroyFramebuffer(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.Framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < VulkanContext.SwapChainImageViews.size(); i++) {
        vkDestroyImageView(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.SwapChain, nullptr);
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
            static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkDevice Renderer::GetLogicalDevice() {
    return VulkanContext.VulkanDevice.LogicalDevice;
}

void Renderer::CreateImageViews() {
    VulkanContext.SwapChainImageViews.resize(VulkanContext.SwapChainImages.size());

    for (size_t i = 0; i < VulkanContext.SwapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = VulkanContext.SwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = VulkanContext.SwapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(VulkanContext.VulkanDevice.LogicalDevice, &createInfo, nullptr, &VulkanContext.SwapChainImageViews[i]) != VK_SUCCESS) {
        }
    }
}

void Renderer::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VulkanContext.SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(VulkanContext.VulkanDevice.LogicalDevice, &renderPassInfo, nullptr, &VulkanContext.RenderPass) != VK_SUCCESS) {
        EM_FATAL("Could not create render pass!");
    } else {
        EM_INFO("created render pass");
    }
}

void Renderer::CreateGraphicsPipeline() {

    const std::vector<char>& vertShaderCode = ReadFile("src/shaders/VertShader.spv");
    const std::vector<char>& fragShaderCode = ReadFile("src/shaders/FragShader.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)VulkanContext.SwapChainExtent.width;
    viewport.height = (f32)VulkanContext.SwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = VulkanContext.SwapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;             
    pipelineLayoutInfo.pSetLayouts = &VulkanContext.DescriptorSetLayout;          

    if (vkCreatePipelineLayout(VulkanContext.VulkanDevice.LogicalDevice, &pipelineLayoutInfo, nullptr, &VulkanContext.PipelineLayout) != VK_SUCCESS) {
        EM_FATAL("Could not create pipeline!");
    } else {
        EM_INFO("CREATED pipeline");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = VulkanContext.PipelineLayout;
    pipelineInfo.renderPass = VulkanContext.RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; 

    if (vkCreateGraphicsPipelines(VulkanContext.VulkanDevice.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &VulkanContext.GraphicsPipeline) != VK_SUCCESS) {
        EM_FATAL("failed to create graphics pipeline!");
    } else {
        EM_INFO("CREATED Graphics pipeline");
    }

    vkDestroyShaderModule(VulkanContext.VulkanDevice.LogicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(VulkanContext.VulkanDevice.LogicalDevice, vertShaderModule, nullptr);
}

VkShaderModule Renderer::CreateShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    EM_INFO("%s", code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(VulkanContext.VulkanDevice.LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        EM_FATAL("Could not create shader module");
    }

    return shaderModule;
}

void Renderer::CreateFrameBuffers() {
    VulkanContext.Framebuffers.resize(VulkanContext.SwapChainImageViews.size());

    for (size_t i = 0; i < VulkanContext.SwapChainImageViews.size(); i++) {
        VkImageView attachments[]{VulkanContext.SwapChainImageViews[i]};

        VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        createInfo.renderPass = VulkanContext.RenderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = VulkanContext.SwapChainExtent.width;
        createInfo.height = VulkanContext.SwapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(VulkanContext.VulkanDevice.LogicalDevice, &createInfo, nullptr, &VulkanContext.Framebuffers[i]) != VK_SUCCESS) {
            EM_FATAL("Could not create framebuffer");
        } else {
            EM_INFO("created frame buffers");
        }
    }
}

void Renderer::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(VulkanContext.VulkanDevice.PhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

    if (vkCreateCommandPool(VulkanContext.VulkanDevice.LogicalDevice, &poolInfo, nullptr, &VulkanContext.CommandPool) != VK_SUCCESS) {
        EM_FATAL("Could not create command pool");
    } else {
        EM_INFO("created command pool");
    }
}

void Renderer::CreateCommandBuffer() {
    VulkanContext.CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = VulkanContext.CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (u32)VulkanContext.CommandBuffers.size();

    if (vkAllocateCommandBuffers(VulkanContext.VulkanDevice.LogicalDevice, &allocInfo, &VulkanContext.CommandBuffers[CurrentFrame]) != VK_SUCCESS) {
        EM_FATAL("Could not create command buffers");
    } else {
        EM_INFO("created command buffer");
    }
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex) {
   VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = VulkanContext.RenderPass;
        renderPassInfo.framebuffer = VulkanContext.Framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = VulkanContext.SwapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanContext.GraphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (f32) VulkanContext.SwapChainExtent.width;
            viewport.height = (f32) VulkanContext.SwapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = VulkanContext.SwapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);            

            VkBuffer vertexBuffers[] = {VulkanContext.VertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, VulkanContext.IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanContext.PipelineLayout, 0, 1, &VulkanContext.DescriptorSets[CurrentFrame], 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
}

void Renderer::CreateSyncObjects() {
    VulkanContext.ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    VulkanContext.RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    VulkanContext.InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(VulkanContext.VulkanDevice.LogicalDevice, &semaphoreInfo, nullptr, &VulkanContext.ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(VulkanContext.VulkanDevice.LogicalDevice, &semaphoreInfo, nullptr, &VulkanContext.RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(VulkanContext.VulkanDevice.LogicalDevice, &fenceInfo, nullptr, &VulkanContext.InFlightFences[i]) != VK_SUCCESS) {
            EM_FATAL("COULD NOT CREATE SEMAPHORES");
        }
    }
}

void Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(VulkanContext.VulkanDevice.LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(VulkanContext.VulkanDevice.LogicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(VulkanContext.VulkanDevice.LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(VulkanContext.VulkanDevice.LogicalDevice, buffer, bufferMemory, 0);
}

void Renderer::CreateIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(Indices[0]) * Indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(VulkanContext.VulkanDevice.LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, Indices.data(), (size_t)bufferSize);
    vkUnmapMemory(VulkanContext.VulkanDevice.LogicalDevice, stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VulkanContext.IndexBuffer, VulkanContext.IndexBufferMemory);

    CopyBuffer(stagingBuffer, VulkanContext.IndexBuffer, bufferSize);

    vkDestroyBuffer(VulkanContext.VulkanDevice.LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(VulkanContext.VulkanDevice.LogicalDevice, stagingBufferMemory, nullptr);
}

void Renderer::CreateVertexBuffer() {

    VkDeviceSize bufferSize = sizeof(Vertices[0]) * Vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(VulkanContext.VulkanDevice.LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, Vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(VulkanContext.VulkanDevice.LogicalDevice, stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VulkanContext.VertexBuffer, VulkanContext.VertexBufferMemory);

    CopyBuffer(stagingBuffer, VulkanContext.VertexBuffer, bufferSize);

    vkDestroyBuffer(VulkanContext.VulkanDevice.LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(VulkanContext.VulkanDevice.LogicalDevice, stagingBufferMemory, nullptr);
}

void Renderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    VulkanContext.UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VulkanContext.UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    VulkanContext.UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VulkanContext.UniformBuffers[i], VulkanContext.UniformBuffersMemory[i]);

        vkMapMemory(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.UniformBuffersMemory[i], 0, bufferSize, 0, &VulkanContext.UniformBuffersMapped[i]);
    }
}

void Renderer::UpdateUniformBuffer(u32 currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj = glm::perspective(glm::radians(45.0f), VulkanContext.SwapChainExtent.width / (float) VulkanContext.SwapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(VulkanContext.UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::CreateDescriptorPool() {
    
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(VulkanContext.VulkanDevice.LogicalDevice, &poolInfo, nullptr, &VulkanContext.DescriptoPool) != VK_SUCCESS) {
        EM_FATAL("failed to create descriptor pool!");
    }
}

void Renderer::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, VulkanContext.DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = VulkanContext.DescriptoPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    VulkanContext.DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(VulkanContext.VulkanDevice.LogicalDevice, &allocInfo, VulkanContext.DescriptorSets.data()) != VK_SUCCESS) {
        EM_FATAL("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = VulkanContext.UniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = VulkanContext.DescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(VulkanContext.VulkanDevice.LogicalDevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = VulkanContext.CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(VulkanContext.VulkanDevice.LogicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(VulkanContext.GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(VulkanContext.GraphicsQueue);

    vkFreeCommandBuffers(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.CommandPool, 1, &commandBuffer);
}

u32 Renderer::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(VulkanContext.VulkanDevice.PhysicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    EM_FATAL("failed to find suitable memory type!");

    return 0;
}

void Renderer::CreateDescriptorSetLayout() {
    
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(VulkanContext.VulkanDevice.LogicalDevice, &layoutInfo, nullptr, &VulkanContext.DescriptorSetLayout) != VK_SUCCESS) {
        EM_FATAL("failed to create descriptor set layout!");
    }
}

void Renderer::Shutdown() {
    CleanSwapChain();
    
    vkDestroyDescriptorPool(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.DescriptoPool, nullptr);
    vkDestroyDescriptorSetLayout(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.DescriptorSetLayout, nullptr);

    vkDestroyBuffer(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.IndexBuffer, nullptr);
    vkFreeMemory(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.IndexBufferMemory, nullptr);

    vkDestroyBuffer(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.VertexBuffer, nullptr);
    vkFreeMemory(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.VertexBufferMemory, nullptr);

     for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.UniformBuffers[i], nullptr);
        vkFreeMemory(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.UniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.DescriptorSetLayout, nullptr);

    vkDestroyPipeline(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.PipelineLayout, nullptr);
    vkDestroyRenderPass(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.RenderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.InFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(VulkanContext.VulkanDevice.LogicalDevice, VulkanContext.CommandPool, nullptr);
    vkDestroyDevice(VulkanContext.VulkanDevice.LogicalDevice, nullptr);

    if (EnableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(VulkanContext.Instance, VulkanContext.DebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(VulkanContext.Instance, VulkanContext.Surface, nullptr);
    vkDestroyInstance(VulkanContext.Instance, nullptr);
}