#include "app.hpp"
#include "debug.hpp"

#include <exception>
#include <string>
#include <format>

int App::Main(int argc, char *argv[])
{
	App app;
	return app.Run();
}

int App::Run()
{
	int exitCode = 0;
	StartupContext c(*this);

	try {
		Startup(c);
		MainLoop(c);
	}
	catch (std::exception &e) {
		trace(std::format("exception caught: {}", e.what()));
		exitCode = 1;
	}
	catch (...) {
		trace("unknown exception caught");
		exitCode = 2;
	}

	// Shutdown(c) will be called when StartupContext leaves scope.

    return exitCode;
}

App::StartupContext::StartupContext(App &app) : _app(app) {}

App::StartupContext::~StartupContext() {
	_app.Shutdown(*this);
}

void App::Startup(StartupContext &c)
{
}

void App::MainLoop(StartupContext &c)
{
}

void App::Shutdown(StartupContext &c)
{
}

