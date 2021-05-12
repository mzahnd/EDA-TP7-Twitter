#include "DisplaySofi.h"
#include <iostream>

using namespace std;

//Se inicializan los datos y se crea el display
LCDB::LCDB() {
    initOk = true;
    //error = lcdError(NO_ERROR);
    spaceDisp = 1;
    cursorPos.row = 0;
    cursorPos.column = 0;

    initOk &= initGraphics();
	
    lcdClear();
}

LCDB::~LCDB() {
    //al_shutdown_font_addon();
    al_destroy_display(display);
}

//Se inicializa la fuente
bool LCDB::lcdInitOk() {

    return initOk;
}

LCDError& LCDB::lcdGetError() {
    return error;
}

//Limpia el display y lleva el cursor al inicio 
bool LCDB::lcdClear() {
    cursorPos.row = 0;
    cursorPos.column = 0;
    for (int i = 0; i < N_COLUMN * N_ROW; i++) {     //se borra el contenido del texto
        data[cursorPos.row][cursorPos.column] = ' ';
        nextPos(cursorPos);
    }
    spaceDisp = 1;
    cursorPos.row = 0;
    cursorPos.column = 0;

    drawDisp();
    return true;
}

//Borra el display desde donde esta el cursor
bool LCDB::lcdClearToEOL() {
    cursorPosition aux;
    aux.row = cursorPos.row;            //se definen coordenadas auxiliares
    aux.column = cursorPos.column;
    while (nextPos(aux)) {
        data[aux.row][aux.column] = ' ';  //cambia el texto desde esa posicion
    }

    drawDisp();
    return 1;
}

//Recibe una letra y la escribe en caso de que no haya nada escrito ahi
basicLCD& LCDB::operator<<(const char c) {
   
   if (data[cursorPos.row][cursorPos.column] == ' ') { 

        data[cursorPos.row][cursorPos.column] = c;
        spaceDisp = nextPos(cursorPos);
    }

    drawDisp();
    return (*this);
}

//Recibe una palabra y la escribe 
basicLCD& LCDB::operator<<(const char* c) {
    int sizeC = 0;
    const char* auxPtr = c;
    while (*auxPtr != 0) {              //se recorre la palabra para calcular el tamano
        sizeC++;
        auxPtr++;
    }
    if (sizeC > (N_ROW) * (N_COLUMN)) {                 //si mide mas del tamano
        int substract = sizeC - (N_ROW) * (N_COLUMN);   //se queda con la ultima parte
        for (int i = 0; i < substract; i++) {
            c++;
        }
    }
    while (*c != '\0')
    {
        data[cursorPos.row][cursorPos.column] = *c;
        c++;
        spaceDisp = nextPos(cursorPos);
    }

    drawDisp();
    return (*this);
}

//Funcion que se encarga de imprimir el texto 
void LCDB::printData() {
    if (this->txtFont == NULL) return;

    for (int i = 0; i < N_ROW; i++) {
        getLine(i);
        al_draw_textf(txtFont, LETTER_COLOR, 2 * U_SIZE, 2 * U_SIZE * i, 0, line);
    }
}

//Esta funcion se encarga de separar el texto en dos lineas, para ser impreso en el display
//Recibe el numero de linea y cambia un dato miembro a lo que se debe imprimir
void LCDB::getLine(int line_) {
    for (int i = 0; i < N_COLUMN; i++) {
        this->line[i] = data[line_][i];
    }
}

//Esta funcion se encarga de moverse a la proxima posicion en el cursor que recibe
//Tiene en consideracion el ancho y largo del display
//Recibe un dato de tipo cursorPosition y cambia sus coordenadas
//Devuelve false si se llego al final del display y true en caso contrario
bool LCDB::nextPos(cursorPosition& pos) {

    if ((pos.column == (N_COLUMN - 1)) && (pos.row == (N_ROW - 1))) {
        spaceDisp = 0;
        return false;
    }
    else if ((pos.column == (N_COLUMN - 2)) && (pos.row == (N_ROW - 1))) {
        pos.column += 1;
        return false;
    }
    else if ((pos.column == (N_COLUMN - 1)) && (pos.row == 0)) {
        pos.column = 0;
        pos.row = 1;
    }
    else {
        pos.column += 1;
    }

    return true; //devuelve si sigue habiendo espacio disponible despues de la operacion
}

//Funcion que mueve el cursor arriba
//Devuelve 0 si no se puede mover mas arriba
bool LCDB::lcdMoveCursorUp() {
    if (cursorPos.row == 0)
    {
        return false;
    }
    else
    {
        cursorPos.row -= 1;
    }

    drawDisp();
    return true;
}

//Funcion que mueve el cursor abajo
//Devuelve 0 si no se puede mover mas abajo
bool LCDB::lcdMoveCursorDown() {
    if (cursorPos.row == (N_ROW - 1))
    {
        return false;
    }
    else
    {
        cursorPos.row += 1;
    }

    drawDisp();
    return true;
}

//Funcion que mueve el cursor a la derecha
bool LCDB::lcdMoveCursorRight() {
    if (cursorPos.row == 0 && cursorPos.column == (N_COLUMN - 1))
    {
        cursorPos.column = 0;
        cursorPos.row = 1;
    }
    else if (cursorPos.row == 1 && cursorPos.column == (N_COLUMN - 1))
    {
        return false;
    }
    else
    {
        this->cursorPos.column += 1;
    }

    drawDisp();
    return true;
}

//Funcion que mueve el cursor a la izquierda
bool LCDB::lcdMoveCursorLeft() {
    if (cursorPos.row == 1 && cursorPos.column == 0)
    {
        cursorPos.column = N_COLUMN - 1;
        cursorPos.row = 0;
    }
    else if (cursorPos.row == 0 && cursorPos.column == 0)
    {
        return false;
    }
    else
    {
        cursorPos.column -= 1;
    }

    drawDisp();
    return true;
}

//Cambia la posicion del cursor
//Devuelve false si se ingresaron coordenadas fuera del display
bool LCDB::lcdSetCursorPosition(const cursorPosition pos) {
    if ((pos.column <= (N_COLUMN - 1) && pos.column >= 0) && (pos.row == 1 || pos.row == 0)) {
        cursorPos.column = pos.column;
        cursorPos.row = pos.row;
    }
    else {
        return false;
    }
    return true;
}

cursorPosition LCDB::lcdGetCursorPosition() {
    return cursorPos;
}

//Funcion que se encarga de imprimir el cursor
void LCDB::printCursor(void){
    al_draw_line(100 + 38 * (cursorPos.column), 2 * (cursorPos.row) * U_SIZE + 20, 100 + 38 * (cursorPos.column), 4 * (cursorPos.row) * U_SIZE + 80, al_map_rgb(0, 0, 0), 2);
}

//Funcion que limpia el display
void LCDB::clearDisp() {
    al_clear_to_color(DISPLAY_COLOR);
}

void LCDB::drawDisp() {
    al_set_target_backbuffer(this->display);
    clearDisp();
    //printCursor();
    printData();
    al_flip_display();
}

bool LCDB::initGraphics() {
    if (!al_init()) {
		fprintf(stderr, " failed to initialize allegro !\n");
	    return false;
	}

    if (!al_init_font_addon()){
        fprintf(stderr, "Failed to initialize the font !\n");
	    return false;
    }
    display = al_create_display(N_COLUMN * U_SIZE, N_ROW * U_SIZE * 2);
    if (!display) {

        fprintf(stderr, "Failed to create display !\n");
	    return false;
    }
    
    txtFont = al_load_ttf_font("res\\love.ttf", U_SIZE, 0);
    if (!this->txtFont) {
        fprintf(stderr, "Failed to initialize the font !\n");
	    return false;
    }

    return true;
}


