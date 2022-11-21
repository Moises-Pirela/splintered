#pragma once

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

struct WindowState {
    HINSTANCE HInstance;
    HWND Hwnd;
    VkSurfaceKHR Surface;
};

class Window {
   public:
   WindowState* State;

   public:
    Window();
    ~Window();

    bool Open(const char* appName, int x, int y, int width, int height);
    void Close();
    void Shutdown();
};
