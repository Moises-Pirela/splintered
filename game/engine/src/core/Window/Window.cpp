#include "Window.h"
#include "core/Logger/Logger.h"
#include "defines.h"
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#define GLFW_INCLUDE_VULKAN
#include <vendor/GLFW/glfw3.h>

LRESULT CALLBACK WindowProc(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

Window::Window(/* args */) {
    EM_INFO("Constructing window");
}

Window::~Window() {
}

bool Window::Open(const char* appName, int x, int y, int width, int height) {
   
    if (!glfwInit())
    {
        EM_FATAL("Could not open window!");
        return FALSE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(width, height, appName, nullptr, nullptr);

    State.GlfwWindow = window;

    return TRUE;
}

void Window::Close() {
    EM_INFO("Closing this window");
}

void Window::Shutdown() {
}