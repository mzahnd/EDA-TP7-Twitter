#include <iostream>
#include <cmath>		/* isless */

#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h> 
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

#include "DisplayMartin.hpp"

#define LCD_ROWS					2
#define LCD_COLS					16
#define LCD_AV_CHAR					((LCD_ROWS)*(LCD_COLS))

#define CURSOR_REAL_POSITION		(this->curPos.row*(LCD_COLS) + this->curPos.column)

#define ISLETTER(a)					(((a) >= 'A' && (a) <= 'Z') || ((a) >= 'a' && (a) <= 'z'))
#define ISNUMBER(a)					((a) >= '0' && (a) <= '9')
#define LETTER_UNCAPS(a)			(((a) >= 'A' && (a) <= 'Z') ? ((a)-'A'+'a') : (a))

#define GUI_FPS						60
#define GUI_KEY_SEEN				1
#define GUI_KEY_RELEASED			2
#define MOUSE_LEFT_BTN				1

#define GUI_DISP_WIDTH				458
#define GUI_DISP_HEIGHT				356
#define GUI_WINDOW_TITLE			"Display de Martin"

#define GUI_TEXT_SEP_X				25.0
#define GUI_TEXT_SEP_Y				30.0

#define GUI_CURSOR_COLOR			al_map_rgba(0, 0, 0, 100)

#define BACKGROUND_PATH				"mzahnd\\res\\displayBoard.png"
// Measured in pixels using the non-scaled bitmap. Automatically updated
#define GUI_DISP_AREA_X1			104
#define GUI_DISP_AREA_Y1			135
#define GUI_DISP_AREA_X2			812
#define GUI_DISP_AREA_Y2			363

#define GUI_FONT_PATH				"mzahnd\\res\\AlphaSmart3000.ttf"

// Special -valid- characters for the display
const char LCD_SPECIAL_CHARS[] = {
	' ', '.', ':', ',', ';', '\'', '\"', '(', ')', 
	'!', '?', '+', '*', '/', '=', '<', '>', '-', '_'
};

enum GUI_BUTTONS {
	GUI_BUTTONS_NONE = -1,
	GUI_BUTTONS_UP = 0,
	GUI_BUTTONS_RIGHT,
	GUI_BUTTONS_DOWN,
	GUI_BUTTONS_LEFT,
	GUI_BUTTONS_CLC,
	GUI_BUTTONS_CURSOR,
	GUI_BUTTONS_N
};

// Measured in pixels using the non-scaled bitmap. Automatically updated
#define GUI_BTN_CORNERS				4
const double btn_pos[GUI_BUTTONS_N][GUI_BTN_CORNERS] = {
	{210, 523, 239, 551},	// UP
	{299, 564, 328, 592},	// RIGHT
	{209, 611, 240, 641},	// DOWN
	{124, 564, 154, 593},	//LEFT
	{617, 568, 646, 596},	// CLC
	{790, 567, 822, 598},	// CURSOR
};

/********************* CONSTRUCTOR *********************/
DisplayMartin::DisplayMartin() 
{
    this->init = true;
    this->curPos = { 0 };
    this->text.clear();
	this->display = NULL;
	this->evQueue = NULL;
	this->background.bitmap = NULL;
	this->background.width = 0;
	this->background.height = 0;
	this->timer.fps = NULL;
	this->timer.cursorBlink = NULL;
	this->request.exit = false;

	this->isStandAlone = false;
	this->mouse.coord_x = 0;
	this->mouse.coord_y = 0;
	this->mouse.btnState = 0;
	this->guiCursorEnabled = true;
	this->cursorState = false;

	this->text.reserve(LCD_AV_CHAR);

	this->init &= initGui();
	if (!this->init) {
		return;
	}

	this->drawDisplay();
}

DisplayMartin::~DisplayMartin() 
{
    this->init = false;
    this->text.clear();
    this->curPos = { 0 };

	al_uninstall_keyboard();
	al_uninstall_mouse();
	al_shutdown_primitives_addon();
	al_shutdown_image_addon();
	//al_shutdown_ttf_addon(); // Breaks everything \_(._.)_/
	al_shutdown_font_addon();

	if (this->timer.fps != NULL) al_destroy_timer(this->timer.fps);
	if (this->timer.cursorBlink != NULL) al_destroy_timer(this->timer.cursorBlink);
	if (this->evQueue != NULL) al_destroy_event_queue(this->evQueue);
	if (this->display != NULL) al_destroy_display(this->display);
}

/********************* PUBLIC METHODS *********************/
bool DisplayMartin::lcdInitOk() {
	return this->init;
}

LCDError& DisplayMartin::lcdGetError()
{
	return this->lcdErr;
}


bool DisplayMartin::lcdClear()
{
	this->text.clear();
	this->curPos.column = 0;
	this->curPos.row = 0;

	if (! this->isStandAlone)
		this->manualUpdateGUI();

	return true;
}
bool DisplayMartin::lcdClearToEOL()
{
	this->text.erase(CURSOR_REAL_POSITION);

	if (! this->isStandAlone)
		this->manualUpdateGUI();

	return true;
}

basicLCD& DisplayMartin::operator<<(const char c)
{
	this->insertText(c);

	if (! this->isStandAlone)
		this->manualUpdateGUI();
	
	return *this;
}

basicLCD& DisplayMartin::operator<<(const char* c)
{
	if (c == NULL) { return *this; }

	this->insertText(c);

	if (! this->isStandAlone)
		this->manualUpdateGUI();

	return *this;
}


bool DisplayMartin::lcdMoveCursorUp()
{
	if (this->curPos.row) {
		this->curPos.row--;

		if (! this->isStandAlone)
			this->manualUpdateGUI();

		return true;
	}
	
	lcdErr.setError("Cursor error.", 
		"The cursor is already on the upper row.", 
		UP_BOUNDARY
	);
	return false;
}

bool DisplayMartin::lcdMoveCursorRight()
{
	if (this->curPos.column == LCD_COLS-1 && this->curPos.row < LCD_ROWS-1) {
		this->curPos.column = 0;
		this->curPos.row++;
	}
	else if (this->curPos.column >= 0 && this->curPos.column < LCD_COLS-1) {
		this->curPos.column++;
	}
	else {
		lcdErr.setError("Cursor error.", 
			"The cursor is already at the end.", 
			RIGHT_BOUNDARY	
		);
		return false;
	}

	if (! this->isStandAlone)
		this->manualUpdateGUI();

	return true;
}

bool DisplayMartin::lcdMoveCursorDown()
{
	if (this->curPos.row >= 0 && this->curPos.row < LCD_ROWS-1) {
		this->curPos.row++;

		if (! this->isStandAlone)
			this->manualUpdateGUI();

		return true;
	}

	lcdErr.setError("Cursor error.", 
		"The cursor is already on the bottom row.", 
		DOWN_BOUNDARY
	);

	return false;
}

bool DisplayMartin::lcdMoveCursorLeft()
{
	if (this->curPos.column == 0 && this->curPos.row) {
		this->curPos.column = LCD_COLS-1;
		this->curPos.row--;
	}
	else if (this->curPos.column) { 
		this->curPos.column--;
	}
	else {
		lcdErr.setError("Cursor error.", 
			"The cursor is already at the beggining of the display.", 
			LEFT_BOUNDARY
		);
		return false;
	}

	if (! this->isStandAlone)
		this->manualUpdateGUI();

	return true;
}


bool DisplayMartin::lcdSetCursorPosition(const cursorPosition pos)
{
	if (pos.row == 0xEA && pos.column == 0xE6) {
		lcdErr.setError("EASTER EGG FOUND.", 
			"Now you have enabled a full operational GUI! Enjoy it.", 
			DISPLAY
		);

		std::cout << "\n" << lcdErr.getErrorName() << "\n" << lcdErr.getErrorDescription() << "\n";

		return this->standAloneGUI();
	}

	if (pos.row >= 0 && pos.row < LCD_ROWS-1 &&
		pos.column >= 0 && pos.column < LCD_COLS-1)
	{
		this->curPos = pos;

		if (! this->isStandAlone)
			this->manualUpdateGUI();

		return true;
	}

	lcdErr.setError("Cursor error.", 
		"Invalid position set for cursor.", 
		OUT_OF_BOUNDS
	);

	return false;
}

cursorPosition DisplayMartin::lcdGetCursorPosition()
{
	return this->curPos;
}

/********************* PRIVATE METHODS *********************/
bool DisplayMartin::isValidChar(const char c)
{
	if (ISLETTER(c) || ISNUMBER(c)) {
		return true;
	}

	for (char v : LCD_SPECIAL_CHARS) {
		if (c == v) {
			return true;
		}
	}
	return false;
}

void DisplayMartin::insertText(const char c)
{
	if (isValidChar(c)) {
		// Add some spaces when cursor is set ahead of the last character
		while (this->text.size() < (size_t)CURSOR_REAL_POSITION) {
			this->text.append(1, ' ');
		}

		this->text.replace(CURSOR_REAL_POSITION, 1, 1, c);
		this->lcdMoveCursorRight();
	}
}

void DisplayMartin::insertText(const char* c) 
{
	if (c == NULL) return;

	std::string valid_c = c;
	
	// Add some spaces when cursor is set ahead of the last character
	while (this->text.size() < (size_t)CURSOR_REAL_POSITION) {
		this->text.append(1, ' ');
	}

	// Validate input
	for (std::size_t i = 0; i < valid_c.size(); i++) {
		if (!isValidChar(valid_c[i])) {
			valid_c.erase(i, 1);
		}
	}

	if (valid_c.length() > LCD_AV_CHAR) {
		lcdErr.setError("Writting error.",
			"Too many characters in input string. Only printing the last of them.",
			WRITING_EXCEEDED
		);
	}
	else if (valid_c.length() + CURSOR_REAL_POSITION > LCD_AV_CHAR) {
		lcdErr.setError("Writting error.",
			"Exceded last column in last row while printing.",
			WRITING_EXCEEDED
		);
	}

	if (valid_c.length() >= LCD_AV_CHAR) {
		this->curPos.column = 0;
		this->curPos.row = 0;

		this->text.assign(valid_c, valid_c.size() - LCD_AV_CHAR, LCD_AV_CHAR);
	}
	else {
		// Divide the string in 2 pieces:
		// One with the remaining available characters after the cursor.
		// The other one,when needed, with those to replace at the beggining of the string.
		std::string portion1 = valid_c.substr(0, LCD_AV_CHAR - CURSOR_REAL_POSITION);

		this->text.replace(CURSOR_REAL_POSITION, portion1.size(), portion1);

		if (valid_c.length() > (size_t)(LCD_AV_CHAR - CURSOR_REAL_POSITION)) {
			std::string portion2;
			portion2.assign(valid_c.substr(portion1.length()));

			this->text.replace(0,  portion2.size(), portion2);

			// Move the cursor properly
            if ((portion2.length() + CURSOR_REAL_POSITION) % LCD_AV_CHAR == 0) {
				this->curPos.row = 0;
				this->curPos.column= 0;
			}
			else {
				this->curPos.row = (int)(portion2.length() / LCD_COLS);
				this->curPos.column = (int)((portion2.length() - this->curPos.row * LCD_COLS));
			}
		}
		else {
			// Move the cursor properly
			int totalNewPosition = portion1.length() + CURSOR_REAL_POSITION;

			if (totalNewPosition % LCD_AV_CHAR == 0) {
				this->curPos.row = 0;
				this->curPos.column= 0;
			}
			else {
				this->curPos.row = (int)(totalNewPosition / LCD_COLS);
				this->curPos.column = (int)(totalNewPosition - this->curPos.row * LCD_COLS);
			}
		}
	}

}

// GUI
bool DisplayMartin::manualUpdateGUI(void)
{
	this->guiCursorEnabled = true;
	this->cursorState = true;
	this->drawDisplay();

	return true;
}

bool DisplayMartin::standAloneGUI(void)
{
	if (!this->init) return false;

	this->isStandAlone = true;

	this->timer.fps = al_create_timer(1.0 / GUI_FPS);
	if (this->timer.fps == NULL) {
		fprintf(stderr, "Unable to create main timer.\n");
		return false;
	}
	this->timer.cursorBlink = al_create_timer(GUI_FPS / GUI_FPS);
	if (this->timer.fps == NULL) {
		fprintf(stderr, "Unable to create cursor blink timer.\n");
		return false;
	}

	al_register_event_source(this->evQueue, al_get_timer_event_source(this->timer.fps));
	al_register_event_source(this->evQueue, al_get_timer_event_source(this->timer.cursorBlink));

	al_start_timer(this->timer.cursorBlink);
	al_start_timer(this->timer.fps);
	
	ALLEGRO_EVENT ev;
	unsigned long lastError = 0;
	while (!this->request.exit) {
		while (al_get_next_event(this->evQueue, &ev)) {
			this->eventDispatcher(ev);
		}

		if (this->lcdErr.getErrorCode() != lastError) {
			lastError = this->lcdErr.getErrorCode();
			std::cout << "Error" << std::endl;
			std::cout << "\tName: " << this->lcdErr.getErrorName() << std::endl;
			std::cout << "\tDescription: " << this->lcdErr.getErrorDescription() << std::endl;
			std::cout << "\tCode: " << lastError << std::endl;
		}
	}

	return true;
}

bool DisplayMartin::eventDispatcher(ALLEGRO_EVENT& ev)
{
	switch (ev.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			this->request.exit = true;
			break;

		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			if (ev.mouse.button == MOUSE_LEFT_BTN && this->mouse.btnState == 0) {
				this->mouse.coord_x = ev.mouse.x;
				this->mouse.coord_y = ev.mouse.y;
				this->mouse.btnState = GUI_KEY_SEEN | GUI_KEY_RELEASED;
			}
			break;

		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			if (ev.mouse.button == MOUSE_LEFT_BTN) {
				this->mouse.btnState &= GUI_KEY_RELEASED;
			}
			break;

		case ALLEGRO_EVENT_KEY_CHAR:
			if (ev.keyboard.unichar >= 32) {
				this->handleKeyboard(ev.keyboard.unichar);
			}
			break;

		case ALLEGRO_EVENT_TIMER:
			if (ev.timer.source == this->timer.cursorBlink) {
				if (this->guiCursorEnabled) {
					this->cursorState= !this->cursorState;
				}
			}
			if (ev.timer.source == this->timer.fps) {
				this->handleMouse();
				this->drawDisplay();
			}
			break;

		default:
			break;
	}
	return true;
}

void DisplayMartin::handleKeyboard(int key)
{
	if (key > 255) return;
	if (key == 127) {  // DEL (Supr) Key
		this->lcdClearToEOL();
	}
	else {
		this->insertText((const char)key);
	}
}

void DisplayMartin::handleMouse(void) 
{
	int btn = isMouseOverButton();

	this->mouse.btnState &= GUI_KEY_SEEN;
	this->clearMouseCoords();
				
	switch (btn) {
		case GUI_BUTTONS_UP:
			this->lcdMoveCursorUp();
			this->resetCursorAnimation();
			break;
		case GUI_BUTTONS_RIGHT:
			this->lcdMoveCursorRight();
			this->resetCursorAnimation();
			break;
		case GUI_BUTTONS_DOWN:
			this->lcdMoveCursorDown();
			this->resetCursorAnimation();
			break;
		case GUI_BUTTONS_LEFT:
			this->lcdMoveCursorLeft();
			this->resetCursorAnimation();
			break;
		case GUI_BUTTONS_CLC:
			this->lcdClear();
			break;
		case GUI_BUTTONS_CURSOR:
			this->guiCursorEnabled = !this->guiCursorEnabled;
			if (this->guiCursorEnabled) {
				this->resetCursorAnimation();
			}
			else {
				al_stop_timer(this->timer.cursorBlink);
			}
			break;
		default:
			break;
	}
}

bool DisplayMartin::initGui(void)
{
	if (!al_init()) {
		fprintf(stderr, " failed to initialize allegro !\n");
		return false;
	}

	// Smooooth. Makes displays flicker :(
	//al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
	//al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
	//al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

	if (!al_init_primitives_addon()) {
		fprintf(stderr, "failed to initialize primitives addon!\n");
		return false;
	}

	if (!al_install_keyboard()) {
		fprintf(stderr, "failed to initialize keyboard!\n");
		return false;
	}

	if (!al_install_mouse()) {
		fprintf(stderr, "failed to initialize mouse!\n");
		return false;
	}

	if (!al_init_image_addon()) {
		fprintf(stderr, "failed to initialize image addon!\n");
		return false;
	}

	if (!al_init_font_addon()) {
		fprintf(stderr, "failed to initialize font addon!\n");
		return false;
	}

	if (!al_init_ttf_addon()) {
		fprintf(stderr, "failed to initialize ttf font addon!\n");
		return false;
	}

	this->display = al_create_display(GUI_DISP_WIDTH, GUI_DISP_HEIGHT);
	if (this->display == NULL) {
		fprintf(stderr, "Unable to create display. Aborting.\n");
		return false;
	}
	al_set_window_title(this->display, GUI_WINDOW_TITLE);

	this->evQueue = al_create_event_queue();
	if (this->evQueue == NULL) {
		fprintf(stderr, "Unable to create event queue.\n");
		return false;
	}
	al_register_event_source(this->evQueue, al_get_display_event_source(this->display));
	al_register_event_source(this->evQueue, al_get_keyboard_event_source());
	al_register_event_source(this->evQueue, al_get_mouse_event_source());

	this->background.bitmap = al_load_bitmap(BACKGROUND_PATH);
	if (this->background.bitmap == NULL) {
		fprintf(stderr, "Unable to load background.\n");
		return false;
	}
	this->background.width = al_get_bitmap_width(this->background.bitmap);
	this->background.height = al_get_bitmap_height(this->background.bitmap);

	// 
	float _height_ratio = GUI_DISP_HEIGHT / this->background.height;
	this->txtFont = al_load_ttf_font(GUI_FONT_PATH, 
		(int) (_height_ratio * (GUI_DISP_AREA_Y2 - GUI_DISP_AREA_Y1 - GUI_TEXT_SEP_Y*3) / 2),
		0);
	if (this->background.bitmap == NULL) {
		fprintf(stderr, "Unable to load font.\n");
		return false;
	}

	return true;
}

bool DisplayMartin::drawDisplay(void)
{
	if (!this->init) return false;

	al_set_target_backbuffer(this->display);

	al_clear_to_color(al_map_rgb(255, 255, 255));

    this->width_ratio = GUI_DISP_WIDTH/this->background.width;
    this->height_ratio = GUI_DISP_HEIGHT/this->background.height;

	// Background
	al_draw_scaled_bitmap(this->background.bitmap, 
		0, 0, this->background.width, this->background.height,
		0, 0, GUI_DISP_WIDTH, GUI_DISP_HEIGHT,
		0);
	

	// Display
	al_draw_filled_rectangle(
		width_ratio*GUI_DISP_AREA_X1, height_ratio*GUI_DISP_AREA_Y1,
		width_ratio*GUI_DISP_AREA_X2, height_ratio*GUI_DISP_AREA_Y2,
		al_map_rgb(125, 123, 54)
	);

	this->drawText();
	this->drawCursor();

	al_flip_display();

	return true;
}

bool DisplayMartin::drawText(void)
{	
	std::size_t txt_pos = 0;

	while (txt_pos < this->text.size()) {
		std::string tmp_txt;

		long long extra_disp_y = 0;
		if (this->text.size() >= LCD_COLS) {
			tmp_txt.assign(this->text.substr(txt_pos, LCD_COLS));
			txt_pos += LCD_COLS;
			extra_disp_y = (long long)(txt_pos/LCD_COLS) - 1;
		}
		else {
			tmp_txt.assign(this->text);
			txt_pos += this->text.size();
		}

		al_draw_text(this->txtFont, al_map_rgb(0, 0, 0),
			width_ratio * (GUI_DISP_AREA_X1 + GUI_TEXT_SEP_X),
			height_ratio * (GUI_DISP_AREA_Y1 
				+ GUI_TEXT_SEP_Y + 0.5)
				+ extra_disp_y * al_get_font_line_height(this->txtFont),
			ALLEGRO_ALIGN_LEFT,
			tmp_txt.c_str()
		);
	}

	return true;
}

bool DisplayMartin::drawCursor(void)
{
	if (!this->guiCursorEnabled || !this->cursorState) return true;

	int text_width = al_get_text_width(this->txtFont, "0");
	int text_height = al_get_font_line_height(this->txtFont);

	double x = width_ratio * (GUI_DISP_AREA_X1 + GUI_TEXT_SEP_X) 
		+ (long long) this->curPos.column * text_width;
	double y = height_ratio * (GUI_DISP_AREA_Y1 + GUI_TEXT_SEP_Y)
		+ (long long) this->curPos.row * text_height;

	al_draw_filled_rectangle(
		x, y,
		x + text_width, y + text_height,
		GUI_CURSOR_COLOR
	);

	return true;
}

int DisplayMartin::isMouseOverButton(void) 
{
	int ans = GUI_BUTTONS_NONE;
	if (this->mouse.btnState == GUI_KEY_SEEN) return ans;

	for (int i = 0; i < GUI_BUTTONS_N; i++) {
		if (isgreaterequal(mouse.coord_x, btn_pos[i][0]*width_ratio) 
			&& isgreaterequal(mouse.coord_y, btn_pos[i][1]*height_ratio)
			&& islessequal(mouse.coord_x, btn_pos[i][2]*width_ratio) 
			&& islessequal(mouse.coord_y, btn_pos[i][3]*height_ratio)) {
			ans = i;
			break;
		}
	}

	return ans;
}

void DisplayMartin::clearMouseCoords(void) 
{
	this->mouse.coord_x = 0;
	this->mouse.coord_y = 0;
	return;
}

void DisplayMartin::resetCursorAnimation(void)
{
	if (this->timer.cursorBlink == NULL) return;

	if (this->guiCursorEnabled) this->cursorState = true;

	al_stop_timer(this->timer.cursorBlink);
	al_start_timer(this->timer.cursorBlink);
}
