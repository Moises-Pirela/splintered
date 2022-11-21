#include "Window.h"
#include "core/Logger/Logger.h"
#include "defines.h"
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>

LRESULT CALLBACK WindowProc(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

Window::Window(/* args */) {
    EM_INFO("Constructing window");
}

Window::~Window() {
}

bool Window::Open(const char* appName, int x, int y, int width, int height) 
{    
    EM_INFO("Opening Window on Windows");

    HINSTANCE hInstance = GetModuleHandleA(0);

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = appName;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                            // Optional window styles.
        appName,                      // Window class
        appName,  // Window text
        WS_OVERLAPPEDWINDOW,          // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        return false;
    }

    WindowState state = {};

    state.HInstance = hInstance;
    state.Hwnd = hwnd;

    State = &state;

    ShowWindow(hwnd, 1);

    return true;
}

void Window::Close()
{
    EM_INFO("Closing this window");
}

void Window::Shutdown()
{
        
}

LRESULT CALLBACK WindowProc(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {

        case WM_ERASEBKGND:

            return 1;

        case WM_CLOSE:
        //TODO: Fire an event on close

            return 0;

            case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

            case WM_SIZE: {
                // RECT r;
                // GetClientRect(hwnd, &r);
                // u32 width = r.right - r.left;
                // u32 height = r.bootm - r.top;

                //TODO: Fire an event on resize
            } break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP: {
                //TODO: Poll keyboard input
                EM_INFO("%d" , msg);
            } break;

            case WM_MOUSEMOVE:{
                //i32 x_position = GET_X_LPARAM(l_param);
                //i32 y_position = GET_Y_LPARAM(l_param);
                //TODO: Poll mouse input
            } break;

            case WM_MOUSEWHEEL:{
                
                i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
                
                if (z_delta != 0){
                    z_delta = (z_delta <0) ? -1 : 1;
                }
                //TODO: Poll mouse wheel input
            }break;

            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:{
                //b8 pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
                //TODO: Poll mouse button input
            }break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}