#include <iostream>

#include "Gui.h"

int main(void)
{
	bool runOk = false;

	GUI programita;

	if (programita.isInit()) {
		runOk = programita.showGUI();
	}

	return runOk;

}