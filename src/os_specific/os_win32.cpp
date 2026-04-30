// do not include pch.h here

#ifdef WIN32

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <dwmapi.h>
#include <windows.h>

#include "os_win32.h"
#include "resource.h"

namespace Mirael::WindowsOnly
{

void customizeMainWindow(GLFWwindow *mainWindow)
{
    HWND hwnd = glfwGetWin32Window(mainWindow);

    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
}

}; // namespace Mirael::WindowsOnly

#endif // WIN32