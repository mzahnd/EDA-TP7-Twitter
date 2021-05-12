#pragma once
#ifndef _GUI_H_
#define _GUI_H_  1

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_allegro5.h>

#include <LCD/LCD.h>

#include "TwitterLCD.h"

#define _USERNAME_BUFF 512

class GUI {
public:
	GUI();
	~GUI();

	bool isInit();
	bool showGUI();

private:
	bool init;
    basicLCD* currentLCD;
	int previousLCD;
	TwitterLCD* tw;

	int twError;

	struct {
		bool isDownloading;
		bool hasDownloaded;
		int tweetsNumber;
		char twitterUsernameBuff[_USERNAME_BUFF];
	} mainWindowData;
	
	ImGuiContext* ctx;
	ALLEGRO_DISPLAY* display;
	ALLEGRO_EVENT_QUEUE* evQueue;

	struct {
		ALLEGRO_TIMER* fps;
		ALLEGRO_TIMER* lcdUpdate;
	} timer;

	struct {
		bool exit;
	} request;

	bool initGui();
	bool eventDispatcher(ALLEGRO_EVENT& ev);
	bool drawMainWindow();
	bool mainWindow();

	bool runTweetFeed(int chosenDisp);
};

#endif /* ! _GUI_H_ */

