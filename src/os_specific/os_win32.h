#pragma once

#ifdef WIN32

#include <GLFW/glfw3.h>

namespace Mirael::WindowsOnly
{

void customizeMainWindow(GLFWwindow *mainWindow);

};

#endif // WIN32
