#pragma once

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#define GLFW_INCLUDE_VULKAN
#include <vendor/GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <vendor/GLFW/glfw3native.h>

struct WindowState {
    GLFWwindow* GlfwWindow;
    VkSurfaceKHR Surface;

    HWND GetHWND()
    {
        return glfwGetWin32Window(GlfwWindow);
    }

    HINSTANCE GetWindow()
    {
        return GetModuleHandle(nullptr);
    }
};

class Window {
   public:
   
   WindowState State;

   public:
    Window();
    ~Window();

    bool Open(const char* appName, int x, int y, int width, int height);
    void Close();
    void Shutdown();
};
