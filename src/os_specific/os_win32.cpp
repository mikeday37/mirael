// do not include pch.h here

#ifdef WIN32

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <optional>
#include <dwmapi.h>
#include <windows.h>

#include "os_win32.h"
#include "resource.h"

namespace Mirael::WindowsOnly
{

namespace
{

std::optional<HICON> hApplicationIcon{};

void loadApplicationIcon()
{
    if (!hApplicationIcon) {
        hApplicationIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    }
}

void setWindowIcon(HWND hwnd)
{
    loadApplicationIcon();
    if (hApplicationIcon && *hApplicationIcon != NULL) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)*hApplicationIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)*hApplicationIcon);
    }
}

}; // namespace

void customizeMainWindow(GLFWwindow *mainWindow)
{
    HWND hwnd = glfwGetWin32Window(mainWindow);
    setWindowIcon(hwnd);

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
}

void customizeSeparatedWindow(GLFWwindow *window)
{
    HWND hwnd = glfwGetWin32Window(window);
    setWindowIcon(hwnd);
}

}; // namespace Mirael::WindowsOnly

#endif // WIN32