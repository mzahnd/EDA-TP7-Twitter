#include <iostream>

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

//#include "..\\lib\\ImGui\imgui.h"
//#include "..\\lib\\ImGui\imgui_impl_allegro5.h"

#include "DisplayMartin.hpp"
#include "DisplaySofi.h"

#include "Gui.h"

#define GUI_FPS						50
#define GUI_LCD_UPDATE_FPS			2
#define GUI_DISP_WIDTH				500
#define GUI_DISP_HEIGHT				150
#define GUI_WINDOW_TITLE			"Twitter Feed"

enum DisplayOptions {
	DISPLAY_OPT_SOFI = 0,
	DISPLAY_OPT_MARTIN,
	DISPLAY_OPT_NDISP
};
/* Uses DisplayOptions enum */
const char* DisplayOptions[DISPLAY_OPT_NDISP] = { "Sofia", "Martin" };

/********************* CONSTRUCTOR *********************/
GUI::GUI()
{
	this->init = true;
	this->twError = TWEETS_OK;
	this->ctx = NULL;
	this->display = NULL;
	this->evQueue = NULL;
	this->timer.fps = NULL;
	this->timer.lcdUpdate = NULL;
	this->request.exit = false;
	this->currentLCD = NULL;
	this->tw = NULL;

	this->mainWindowData.isDownloading = false;
	for (int i = 0; i < _USERNAME_BUFF; i++) {
		this->mainWindowData.twitterUsernameBuff[i] =  0;
	}
	this->mainWindowData.tweetsNumber = 50;
	this->mainWindowData.hasDownloaded = false;

	this->tw = new TwitterLCD();
	this->init &= this->tw->isInit();
	if (!this->init)
		return;

	this->init &= initGui();
	if (!this->init)
		return;

	return;
}

GUI::~GUI()
{
	this->init = false;

	ImGui_ImplAllegro5_Shutdown();
	ImGui::DestroyContext(this->ctx);

	al_uninstall_keyboard();
	al_uninstall_mouse();
	al_shutdown_primitives_addon();
	al_shutdown_image_addon();
	al_shutdown_font_addon();

	if (this->timer.fps != NULL) al_destroy_timer(this->timer.fps);
	if (this->timer.lcdUpdate != NULL) al_destroy_timer(this->timer.lcdUpdate);
	if (this->evQueue != NULL) al_destroy_event_queue(this->evQueue);
	if (this->display != NULL) al_destroy_display(this->display);

	if (this->tw != NULL) delete this->tw;
	if (this->currentLCD != NULL) delete this->currentLCD;
}


/********************* PUBLIC METHODS *********************/
bool GUI::isInit()
{
	return this->init;
}

bool GUI::showGUI()
{
	if (!this->init)
		return false;

	ALLEGRO_EVENT ev;

	this->timer.fps = al_create_timer(1.0 / GUI_FPS);
	if (this->timer.fps == NULL) {
		fprintf(stderr, "Unable to create main timer.\n");
		return false;
	}
	this->timer.lcdUpdate = al_create_timer(1.0 / GUI_LCD_UPDATE_FPS);
	if (this->timer.lcdUpdate == NULL) {
		fprintf(stderr, "Unable to create LCD timer.\n");
		return false;
	}

	al_register_event_source(this->evQueue, al_get_timer_event_source(this->timer.fps));
	al_register_event_source(this->evQueue, al_get_timer_event_source(this->timer.lcdUpdate));

	al_start_timer(this->timer.fps);
	al_start_timer(this->timer.lcdUpdate);

	while (!this->request.exit) {
		while (al_get_next_event(this->evQueue, &ev)) {
			this->eventDispatcher(ev);
		}
	}

	return true;
}

/********************* PRIVATE METHODS *********************/
bool GUI::initGui()
{
	/* Allegro */
	if (!al_init()) {
		fprintf(stderr, " failed to initialize allegro !\n");
		return false;
	}

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

	/* ImGui */

	IMGUI_CHECKVERSION();
	this->ctx = ImGui::CreateContext();
	ImGui_ImplAllegro5_Init(display);
	ImGui::StyleColorsDark();

	return true;
}

bool GUI::eventDispatcher(ALLEGRO_EVENT& ev)
{
	/* Let ImGui handle the event*/
	ImGui_ImplAllegro5_ProcessEvent(&ev);

	switch (ev.type) {
	case ALLEGRO_EVENT_DISPLAY_CLOSE:
		this->request.exit = true;
		break;

	case ALLEGRO_EVENT_TIMER:
		if (ev.timer.source == this->timer.fps) {
			this->drawMainWindow();
		}
		if (ev.timer.source == this->timer.lcdUpdate) {
			this->tw->updateDisplay();
		}
		al_flip_display();
		break;

	default:
		break;
	}
	return true;
}

bool GUI::drawMainWindow()
{
	bool retVal = true;
	
	ImGui_ImplAllegro5_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

	retVal = this->mainWindow();

	ImGui::Render();

	al_set_target_backbuffer(this->display);
	al_clear_to_color(al_map_rgba_f(1, 1, 0.8, 1));

	ImGui_ImplAllegro5_RenderDrawData(ImGui::GetDrawData());

	return retVal;
}

bool GUI::mainWindow()
{
	bool retVal = true;
	ImGuiWindowFlags window_flags = 0;

	static int selectedDisplay = 0;

	struct ImVec4 errorMsgColor;
	std::string errorMsg;

	window_flags |= ImGuiWindowFlags_NoTitleBar;		/* No TitleBar */
	window_flags |= ImGuiWindowFlags_NoScrollbar;		/* No ScrollBar */
	window_flags |= ImGuiWindowFlags_NoMove;			/* No Move */
	window_flags |= ImGuiWindowFlags_NoResize;			/* No Resize */
	window_flags |= ImGuiWindowFlags_NoCollapse;		/* No Collapse */

	ImGui::SetNextWindowSize(ImVec2(GUI_DISP_WIDTH, GUI_DISP_HEIGHT), ImGuiCond_Always); //Aca pongo tamaño de la pantalla

	ImGui::Begin(GUI_WINDOW_TITLE, NULL, window_flags);

	ImGui::InputText("Twitter username", this->mainWindowData.twitterUsernameBuff, _USERNAME_BUFF);
	
	ImGui::InputInt("Number of tweets", &(this->mainWindowData.tweetsNumber));
	if (this->mainWindowData.tweetsNumber <= 0) {
		this->mainWindowData.tweetsNumber = 1;
	}
	if (this->mainWindowData.tweetsNumber > 200) {
		this->mainWindowData.tweetsNumber = 200;
	}

	ImGui::Combo("Display", &selectedDisplay, DisplayOptions, DISPLAY_OPT_NDISP);

	if (! this->mainWindowData.isDownloading) {
		if (ImGui::Button("Download Tweets")) {
			if (this->mainWindowData.tweetsNumber > 0					/* At least one tweet */
				&& this->mainWindowData.twitterUsernameBuff[0] != 0) {	/* At least one char */

				retVal = this->runTweetFeed(selectedDisplay);
				this->mainWindowData.isDownloading = retVal;

				if (!retVal) {
					std::cout << "Unable to download tweets :(";
					this->twError = this->tw->getState();
				}
			}
		}
	}
	else {
		if (ImGui::Button("Cancel Download")) {
			retVal = this->tw->interruptDownload();
			if (!retVal) {
					std::cout << "Problem while aborting tweets download :(";
					this->twError = this->tw->getState();
			}
			
			this->mainWindowData.isDownloading = false;
		}

		bool downloading = this->tw->getDownloadState();
		if (!downloading &&  this->twError == FINISHED_OK) {
			this->mainWindowData.hasDownloaded = true;
			this->mainWindowData.isDownloading = false;

			this->tw->showTweetsOnDisplay();
		}

		this->twError = this->tw->getState();
	}
	
	if (this->tw != NULL && this->tw->hasTweets()) {
		if (ImGui::Button("< Previous")) {
			this->tw->setPrevTweet();
		}

		ImGui::SameLine();
		if (ImGui::Button("Next >")) {
			this->tw->setNextTweet();
		}
	}

	switch (this->twError)
	{
	case TWEETS_OK:
		errorMsg = "Everything seems fine.";
		errorMsgColor = { 0.0f, 152.0 / 255.0f, 70.0 / 255.0f, 1.0f }; /* Green */
		break;
	case INTERNAL_ERROR:
		errorMsg = "Unexpected error \\_(._.)_/";
		errorMsgColor = { 125.0/255.0f, 33.0/255.0f, 129.0/255.0f, 1.0f }; /* Violet */
		break;
	case INVALID_ARGS:
		errorMsg = "Developer error -_-";
		errorMsgColor = { 125.0/255.0f, 33.0/255.0f, 129.0/255.0f, 1.0f }; /* Violet */
		break;
	case DOWNLOADING:
		errorMsg = "Downloading tweets...";
		errorMsgColor = { 0.0f, 152.0 / 255.0f, 70.0 / 255.0f, 1.0f }; /* Green */
		break;
	case COULD_NOT_CONNECT:
		errorMsg = "Unable to connect with Twitter.";
		errorMsgColor = { 0.0f, 100.0 / 255.0f, 85.0 / 255.0f, 1.0f }; /* Red */
		break;
	case USER_IS_PRIVATE:
		errorMsg = "The requested user has a private account.";
		errorMsgColor = { 239.0/255.0f, 127.0 / 255.0f, 85.0 / 26.0f, 1.0f }; /* Orange */
		break;
	case USER_NOT_EXISTS:
		errorMsg = "The requested user does not exists.";
		errorMsgColor = { 239.0/255.0f, 127.0 / 255.0f, 85.0 / 26.0f, 1.0f }; /* Orange */
		break;
	case  USER_HAS_NO_TWEETS:
		errorMsg = "The requested has no tweets.";
		errorMsgColor = { 239.0/255.0f, 127.0 / 255.0f, 85.0 / 26.0f, 1.0f }; /* Orange */
		break;
	case USER_HAS_LESS_TWEETS:
		errorMsg = "The requested has less tweets than requested. ("
			+ std::to_string(this->tw->tweetsAvailable())
			+ " out of "
			+ std::to_string(this->tw->tweetsRequested()) + ")";
		errorMsgColor = { 239.0/255.0f, 127.0 / 255.0f, 85.0 / 26.0f, 1.0f }; /* Orange */
		break;
	case TWITTER_ERROR:
		errorMsg = "Twitter returned an unknown error \\_(._.)_/";
		errorMsgColor = { 0.0f, 100.0 / 255.0f, 85.0 / 255.0f, 1.0f }; /* Red */
		break;
	default:
		break;
	}

	ImGui::TextColored(errorMsgColor, errorMsg.c_str());

	ImGui::End();

	return retVal;
}

bool GUI::runTweetFeed(int chosenDisp)
{
	if (!this->tw || this->tw && !this->tw->isInit()){
		if (this->tw) delete this->tw;

		this->tw = new TwitterLCD();

		if (!this->tw->isInit())
			return false;
	}

	switch (chosenDisp) {
	case DISPLAY_OPT_MARTIN:
		if (this->currentLCD == NULL) {
			this->currentLCD = new DisplayMartin();
		}
		else {
			this->currentLCD->lcdClear();
		}

		break;
	case DISPLAY_OPT_SOFI:
		if (this->currentLCD == NULL) {
			this->currentLCD = new LCDB();
		}
		else {
			this->currentLCD->lcdClear();
		}
		break;
	default:
		break;
	}
	
	if (!this->currentLCD->lcdInitOk()) {
		return false;
	}

	int downloadStatus = this->tw->startDownloading(
		std::string(this->mainWindowData.twitterUsernameBuff),
		this->currentLCD, 
		this->mainWindowData.tweetsNumber
	);

	bool retVal = false;
	if (downloadStatus == DOWNLOADING) {
		retVal = true;
	}
	return retVal;
}

/********************* STATIC METHODS *********************/
