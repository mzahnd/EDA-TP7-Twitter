#pragma once
#ifndef _DISPLAYMARTIN_H_
#define _DISPLAYMARTIN_H_	1

#include <string>

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>

#include <LCD/LCD.h>

class DisplayMartin : public basicLCD
{
public:
	DisplayMartin();
	~DisplayMartin();

	virtual bool lcdInitOk();
	virtual LCDError& lcdGetError();

	virtual bool lcdClear();
	virtual bool lcdClearToEOL();

	virtual basicLCD& operator<<(const char c);
	virtual basicLCD& operator<<(const char* c);

	virtual bool lcdMoveCursorUp();
	virtual bool lcdMoveCursorRight();
	virtual bool lcdMoveCursorDown();
	virtual bool lcdMoveCursorLeft();

	virtual bool lcdSetCursorPosition(const cursorPosition pos);
	virtual cursorPosition lcdGetCursorPosition();

private:
	bool init;
	LCDError lcdErr;
	cursorPosition curPos;
	std::string text;

	bool isValidChar(const char c);
	void insertText(const char c);
	void insertText(const char * c);

	// GUI Stuff
	ALLEGRO_DISPLAY* display;
	ALLEGRO_EVENT_QUEUE* evQueue;
	struct {
		ALLEGRO_BITMAP* bitmap;
		float width;
		float height;
	} background;

	struct {
		ALLEGRO_TIMER* fps;
		ALLEGRO_TIMER* cursorBlink;
	} timer;
	ALLEGRO_FONT* txtFont;

	struct {
		double coord_x;
		double coord_y;
		unsigned btnState;
	} mouse;

	int keys[ALLEGRO_KEY_MAX];

	struct {
		bool exit;
	} request;

	double width_ratio;
	double height_ratio;

	bool guiCursorEnabled;
	bool cursorState;

	bool isStandAlone;

	bool initGui(void);

	bool standAloneGUI(void); // Easter egg :)

	bool manualUpdateGUI(void);
	bool drawDisplay(void);
	bool drawText(void);
	bool drawCursor(void);

	bool eventDispatcher(ALLEGRO_EVENT& ev);
	int isMouseOverButton(void);
	void clearMouseCoords(void);
	void resetCursorAnimation(void);

	void handleKeyboard(int key);
	void handleMouse(void);
};
#endif /* _DISPLAYMARTIN_H_ */