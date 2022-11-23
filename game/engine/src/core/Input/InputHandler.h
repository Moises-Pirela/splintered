#pragma once

#include "core/Logger/Logger.h"
#include "defines.h"
#include "core/Window/Window.h"

#include <vendor/GLFW/glfw3.h>

struct MousePosition
{
    f64 X;
    f64 Y;
};

class Input {
    
    public:
    static void Init(GLFWwindow* window);
    static void Handle();
    static MousePosition GetMousePosition();
};
