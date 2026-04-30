#pragma once

namespace Mirael {

class Settings {
public:
	static const char *windowName() {return "Settings";}
	void show(bool &open);
};

};