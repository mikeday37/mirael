#pragma once

class App {
public:
	[[nodiscard]] static int Main(int argc, char *argv[]);

private:
	int Run();

	struct StartupContext;

	void Startup(StartupContext &c);
	void MainLoop(StartupContext &c);
	void Shutdown(StartupContext &c);

	struct StartupContext {
		App &_app;

		StartupContext(App &app);
		~StartupContext();

		// useful stuff for tracking what needs to be passed around, what's been created so far and needs to be destroyed, etc.
	};
};
