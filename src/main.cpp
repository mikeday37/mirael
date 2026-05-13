#include "pch.h"

#include <cstdlib>
#include <iostream>

#include "app.h"

int main()
{
#ifdef NDEBUG
    try {
#endif

        Mirael::App app;
        app.run();

#ifdef NDEBUG
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
#endif

    return EXIT_SUCCESS;
}
