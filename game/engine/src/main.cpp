#include <iostream>
#include "core/Logger/Logger.h"
#include "core/Renderer/Renderer.h"
#include "defines.h"
#include "core/Window/Window.h"
#include <windows.h>

const i32 WIDTH = 800;
const i32 HEIGHT = 600;
const i32 SPRITE_SIZE = 64;

int WINAPI main() {
    Window mainWindow;
    Renderer mainRenderer;

    bool isRunning;

    if (!mainWindow.Open("Splintered - Vulkan", 100, 100, 800, 600)) {
        return -1;
    }

    

    if (!mainRenderer.Initialize("Splintered", &mainWindow)) {
        return -1;
    }

    isRunning = true;

    while (isRunning) 
    {
        //EM_INFO("Run");
    }

    mainRenderer.Shutdown();
    mainWindow.Shutdown();

    return 0;
}