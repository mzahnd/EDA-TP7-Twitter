#ifndef _DISPLAY_SOFI_H
#define _DISPLAY_SOFI_H

#include <LCD/LCD.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_ttf.h"
#include <allegro5/allegro_primitives.h>
#include <string>


#define N_ROW 2
#define N_COLUMN 16
#define U_SIZE 50
#define DISPLAY_COLOR al_map_rgb(255, 0, 255)
#define LETTER_COLOR al_map_rgb(0, 0, 0)

class LCDB : public basicLCD
{
public:
	LCDB();
	virtual ~LCDB();

	virtual bool lcdInitOk();
	virtual bool lcdClear();
	virtual bool lcdClearToEOL();
	virtual LCDError& lcdGetError();
	virtual basicLCD& operator<<(const char c);
	virtual basicLCD& operator<<(const char* c);
	virtual bool lcdMoveCursorUp();
	virtual bool lcdMoveCursorDown();
	virtual bool lcdMoveCursorRight();
	virtual bool lcdMoveCursorLeft();
	virtual bool lcdSetCursorPosition(const cursorPosition pos);
	virtual cursorPosition lcdGetCursorPosition();

private:
	ALLEGRO_DISPLAY* display;
	ALLEGRO_FONT* txtFont;
	bool initGraphics();

	char data[N_ROW][N_COLUMN]; //matriz donde se encuentra todo el texto del display
	void getLine(int nLine);
	void printData();
	LCDError error;
	bool initOk;
	cursorPosition cursorPos;
	void clearDisp();
	void drawDisp();
	bool nextPos(cursorPosition& pos);
	bool spaceDisp;
	void printCursor(void);
	char line[N_COLUMN]; //linea que va a ser impresa en el display
};

#endif#pragma once