#include "pch.h"

#include <iostream>

#include "app.h"

int main()
{
    try {
        Mirael::App app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
