
#include "RoccoLCD.h"

#include <iostream>
#include <string>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>


/*Hay muchos printScreen() comentados porque no eran utiles en este caso y hacian muy lento el codigo. Ahora solo actualizo 
  La pantalla Cuando el cursor se mueve a la derecha o hay un clear*/

RoccoLCD::RoccoLCD()
	: theCursorPosition{ 0, 0 },
	display(nullptr), bitmap(nullptr), font(nullptr), texto("                                ")
{
	error.setError("OK", "OK", 1);
	bitmap = al_load_bitmap("res\\16x2_LCD_displayF.jpg");
	font = al_load_ttf_font("res\\DS-DIGI.ttf", FONT_SIZE, 0);
	if (bitmap == nullptr || font == nullptr) {
		std::cerr << "Could not find resource files!" << std::endl;
		error.setError("Load_resource_error", "Could not load Resource files", ERROR_RESOURCE_FILE_ENUM);
	}
	display = al_create_display(al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap));



	printScreen();
}

RoccoLCD::~RoccoLCD() {
	if (al_is_system_installed()) {
		al_destroy_bitmap(bitmap);
		al_destroy_display(display);
		al_destroy_font(font);
	}
}

bool RoccoLCD::lcdInitOk() {
	if (error.getErrorCode() == NO_ERROR_ENUM) {
		return true;
	}
	else {
		return false;
	}
}

LCDError& RoccoLCD::lcdGetError() {
	return error;
}

bool RoccoLCD::lcdClear() {
	texto = "                                ";
	theCursorPosition.row = 0;
	theCursorPosition.column = 0;
	printScreen();
	return true;
}

bool RoccoLCD::lcdClearToEOL() {
	for (unsigned int i = theCursorPosition.column; i < COLUMS; i++) {
		texto[i] = ' ';
	}
	printScreen();
	return true;
}

basicLCD& RoccoLCD::operator<<(const char c) {
	if (c >= ' ' && c <= '~') {
		texto[theCursorPosition.row * COLUMS + theCursorPosition.column] = c;
	}
	lcdMoveCursorRight();
	printScreen();
	return *this;
}

basicLCD& RoccoLCD::operator<<(const char* c) {
	std::string string = c;
	

	for ( int i = 0; i < string.length(); i++) {
		if (c[i] < ' ' || c[i] > '~') { //Salteo caracteres invalidos
			continue;
		}
		texto[theCursorPosition.row * COLUMS + theCursorPosition.column] = c[i];
		lcdMoveCursorRight();
		//printScreen();
	}
	return *this;

}

bool RoccoLCD::lcdMoveCursorUp() {
	if (theCursorPosition.row <= 0) {
		//printScreen();
		return false;
	}
	else {
		theCursorPosition.row--;
		//printScreen();
		return true;
	}
}

bool RoccoLCD::lcdMoveCursorDown() {
	if (theCursorPosition.row >= 1) {
		//printScreen();
		return false;
	}
	else {
		theCursorPosition.row++;
		//printScreen();
		return true;
	}
}

bool RoccoLCD::lcdMoveCursorRight() {
	if (theCursorPosition.column == COLUMS - 1 && theCursorPosition.row == ROWS - 1) {
		theCursorPosition.column = 0;
		theCursorPosition.row = 0;
		printScreen();
		return true;
	}
	else if (theCursorPosition.column == COLUMS - 1) {
		theCursorPosition.column = 0;
		theCursorPosition.row++;
		printScreen();
		return true;
	}
	else {
		theCursorPosition.column++;
		printScreen();
		return true;
	}

}

bool RoccoLCD::lcdMoveCursorLeft() {
	if (theCursorPosition.column == 0 && theCursorPosition.row == 0) {
		//printScreen();
		return false;
	}
	
	else if (theCursorPosition.row == ROWS - 1 && theCursorPosition.column == 0) {
		theCursorPosition.column = COLUMS - 1;
		theCursorPosition.row = 0;
		//printScreen();
		return true;
	}
	else {
		theCursorPosition.column--;
		//printScreen();
		return true;
	}
	
}
bool RoccoLCD::lcdSetCursorPosition(const cursorPosition pos) {
	if (pos.column >= COLUMS -1 || pos.column < 0) {
		return false;
	}
	else if (pos.row >= ROWS-1 || pos.row < 0) {
		return false;
	}
	else {
		theCursorPosition = pos;
		return true;
	}
	
}

cursorPosition RoccoLCD::lcdGetCursorPosition() {
	return theCursorPosition;
}

void RoccoLCD::printScreen() {
	al_set_target_backbuffer(display);

	al_draw_bitmap(bitmap, 0.0, 0.0, 0);
	for (unsigned int i = 0; i < ROWS; i++) {
		for (unsigned int j = 0; j < COLUMS; j++) {
			al_draw_textf(font, al_map_rgb(0, 0, 0), HORIZONTAL_OFFSET + j * SPACE_BETWEEN_CHARS, VERTICAL_OFFSET + i * SPACE_BETWEEN_ROWS, 0, "%c", texto[(i * COLUMS) + j]);
		}
	}
	al_flip_display();
	return;
}

/*
RoccoLCD::RoccoLCD()
	: lcdError("OK", "Funciona", 1), theCursorPosition{ 0 , 0 },
	display(nullptr), bitmap(nullptr), font(nullptr), texto("                                ")
{

	bitmap = al_load_bitmap("16x2_LCD_displayF.jpg");
	font = al_load_ttf_font("DS-DIGI.ttf", FONT_SIZE, 0);
	if (bitmap == nullptr || font == nullptr) {
		std::cerr << "Could not find resource files!" << std::endl;
		//set_error("Load_resource_error", "Could not load Resource files", ERROR_RESOURCE_FILE_ENUM);
	}
	display = al_create_display(al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap));

	printScreen();
}*/

