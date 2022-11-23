#include "InputHandler.h"
#include "defines.h"
#include "core/Logger/Logger.h"
#include "core/Window/Window.h"
#include <vendor/GLFW/glfw3.h>

static void HandleKeyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
      if (key == GLFW_KEY_E && action == GLFW_PRESS)
        EM_INFO("Press e");
}

static void HandleMouseInput(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
        EM_INFO("Press left");
    }

    if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS) {
        EM_INFO("Press right");
    }
}

static MousePosition MousePos;
static void HandleMousePosition(GLFWwindow* window, double xpos, double ypos)
{
    MousePos.X = xpos;
    MousePos.Y = ypos;
}

void Input::Init(GLFWwindow* window)
{
    glfwSetKeyCallback(window, HandleKeyboardInput);
    glfwSetCursorPosCallback(window, HandleMousePosition);
    glfwSetMouseButtonCallback(window, HandleMouseInput);
}

void Input::Handle()
{
    glfwPollEvents();
}

MousePosition Input::GetMousePosition()
{
    return MousePos;
}