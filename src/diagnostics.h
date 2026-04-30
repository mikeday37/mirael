#pragma once

namespace Mirael {

class Diagnostics {
public:
	static const char *windowName() {return "Diagnostics";}
	void show(bool &open);
};

};