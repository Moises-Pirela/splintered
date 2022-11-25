#include <iostream>
#include "core/Logger/Logger.h"
#include "core/Renderer/Renderer.h"
#include "defines.h"
#include "core/Window/Window.h"
#include "core/Input/InputHandler.h"
#include <windows.h>

const i32 WIDTH = 800;
const i32 HEIGHT = 600;
const i32 SPRITE_SIZE = 64;

int WINAPI main() {

    Window mainWindow;
    Renderer mainRenderer;


    if (!mainWindow.Open("Splintered - Vulkan", 0, 0, 800, 600)) {
        return -1;
    }

    if (!mainRenderer.Initialize("Splintered", &mainWindow)) {
        return -1;
    }

    Input::Init(mainWindow.State.GlfwWindow);

    while(!glfwWindowShouldClose(mainWindow.State.GlfwWindow)) 
    {
        Input::Handle();
        mainRenderer.Draw();
    }

    VkDevice device = mainRenderer.GetLogicalDevice();
    vkDeviceWaitIdle(device);

    glfwDestroyWindow(mainWindow.State.GlfwWindow);

    glfwTerminate();

    mainRenderer.Shutdown();

    return 0;
}