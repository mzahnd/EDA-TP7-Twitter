#ifndef __ROCCOLCD__
#define __ROCCOLCD__

//#include "../TP6/Allegro.h"

#include <LCD/LCD.h>
#include <LCD/LCDError.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#include <string>
#include <iostream>

#define LCD_WIDTH 643
#define LCD_HEIGHT 294

#define COLUMS 16
#define ROWS 2

#define VERTICAL_OFFSET 150
#define HORIZONTAL_OFFSET 120
#define FONT_SIZE 50
#define SPACE_BETWEEN_CHARS 46 
#define SPACE_BETWEEN_ROWS 80

enum ERRORS_ROCCO { NO_ERROR_ENUM = 1, ERROR_RESOURCE_FILE_ENUM };

class RoccoLCD : public basicLCD {
public:

	RoccoLCD();

	~RoccoLCD();

	/*=====================================================
	* Name: lcdInitOk
	* Entra: -
	* Resulta: No genera ning�n cambio en el display.
	* Devuelve en su nombre �true� si el display se inicializ�
	* correctamente (el constructor no tuvo errores) o �false
	* en caso contrario.
	*=====================================================*/
	bool lcdInitOk() override; //

   /*=====================================================
   * Name: lcdGetError
   * Entra: -
   * Resulta: No genera ning�n cambio en el display.
   * Devuelve en su nombre un lcdError&
   *=====================================================*/
	LCDError& lcdGetError() override;//

   /*=====================================================
   * Name: lcdClear
   * Entra: -
   * Resulta: Borra el display y el cursor va a HOME
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdClear() override; 

   /*=====================================================
   * Name: lcdClearToEOL
   * Entra: -
   * Resulta: Borra el display desde la posici�n actual
   * del cursor hasta el final de la l�nea.
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdClearToEOL() override; 

   /*=====================================================
   * Name: operator<<()
   * Entra: Un car�cter
   * Resulta: Pone el car�cter en la posici�n actual
   * del cursor del display y avanza el cursor a la pr�xima
   * posici�n respetando el gap (si el car�cter no es imprimible
   * lo ignora)
   *
   * Devuelve en su nombre una referencia a un basicLCD que permite
   * encascar la funci�n:
   * basicLCD lcd;
   * lcd << �a� << �b� << �c�;
   *=====================================================*/
	basicLCD& operator<<(const char c) override; 

   /*=====================================================
   * Name: operator<<()
   * Entra: Una cadena de caracteres NULL terminated
   * Resulta: imprime la cadena de caracteres en la posici�n actual
   * del cursor y avanza el cursor al final de la cadena respetando
   * el gap (si alg�n car�cter no es imprimible lo ignora). Si recibe una
   * cadena de m�s de 32 caracteres, muestra los �ltimos 32 en el display.
   *
   * Devuelve en su nombre una referencia a un basicLCD que permite
   * encascar la funci�n:
   * basicLCD lcd;
   * lcd << �Hola� << � � << �Mundo�;
   *=====================================================*/
	basicLCD& operator<<(const char* c) override; 

   /*=====================================================
   * Name: lcdMoveCursorUp
   *
   * Entra: -
   * Resulta: Pasa el cursor a la primera l�nea del display sin
   * alterar la columna en la que estaba.
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdMoveCursorUp() override; 

   /*=====================================================
   * Name: lcdMoveCursorDown
   *
   * Entra: -
   * Resulta: Pasa el cursor a la segunda l�nea del display sin
   * alterar la columna en la que estaba.
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdMoveCursorDown() override; 

   /*=====================================================
   * Name: lcdMoveCursorRight
   *
   * Entra: -
   * Resulta: Avanza el cursor una posici�n
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdMoveCursorRight() override; // 

   /*=====================================================
   * Name: lcdMoveCursorLeft
   *
   * Entra: -
   * Resulta: Retrocede el cursor una posici�n
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdMoveCursorLeft() override; //  

   /*=====================================================
   * Name: lcdSetCursorPosition
   * Entra: Recibe una estructura tipo cursorPosition
   * Resulta: Posiciona el cursor en la posici�n dada
   * por row y column. row[0-1] col[0-19]. Ante un valor inv�lido
   * de row y/o column ignora la instrucci�n (no hace nada).
   *
   * Devuelve en su nombre �true� si fue satisfactoria �false�
   * en caso contrario.
   *=====================================================*/
	bool lcdSetCursorPosition(const cursorPosition pos);

	/*=====================================================
	* Name: lcdGetCursorPosition
	* Entra: -
	* Resulta: Devuelve la posici�n actual del cursor.
	*
	*
	* Devuelve una estructura tipo cursorPosition
	*=====================================================*/
	cursorPosition lcdGetCursorPosition() override;

	void printScreen();

private:

	LCDError error;
	cursorPosition theCursorPosition;

	ALLEGRO_DISPLAY* display;
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_FONT* font;

	std::string texto;


};




#endif __ROCCOLCD__

