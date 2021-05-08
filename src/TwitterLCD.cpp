#include <cmath>		/* isgreater */
#include <exception>
#include <iostream>

#include <nlohmann/json.hpp>

#include "TwitterLCD.h"

#define TWITTER_API_QUERY	"https://api.twitter.com/1.1/statuses/user_timeline.json"
#define TWITTER_API_NAME	"screen_name="
#define TWITTER_API_COUNT	"count="
#define TWITTER_API_KEY		"HCB39Q15wIoH61KIkY5faRDf6"
#define TWITTER_API_SECRET	"7s8uvgQnJqjJDqA6JsLIFp90FcOaoR5Ic41LWyHOic0Ht3SRJ6"

#define LCD_COLS			(16)
#define ANIMATION_TIME_S	(1.0)

static size_t curlReadCallBack(void* data, size_t size, size_t nmemb, void* userp);

/********************* CONSTRUCTOR *********************/
TwitterLCD::TwitterLCD(void)
{
	this->init = true;

	this->nTweets = 0;
	this->tweets.clear();
	this->disp = NULL;

	/* curl */
	this->curl = NULL;
	this->multiHandle = NULL;

	this->curlMAns = CURLM_INTERNAL_ERROR;

	this->downloading = false;;
	this->stillRunning = 0;

	/* Twitter */
	this->rawData.clear();
	this->token.clear();

	/* Display animation */
	this->animationCLK = 0;
	this->animationTick = 0;

	this->init &= this->getToken();

	return;
}

TwitterLCD::~TwitterLCD(void)
{
	this->init = false;
	return;
}

/********************* PUBLIC METHODS *********************/
bool TwitterLCD::isInit()
{
	return this->init;
}

int TwitterLCD::startDownloading(std::string username, basicLCD* display, unsigned long long n_tweets)
{
	if (!this->init)
		return INTERNAL_ERROR;

	if (username.empty() || display == NULL || n_tweets == 0 )
		return INVALID_ARGS;

	this->curl = curl_easy_init();
	this->multiHandle = curl_multi_init();

	if (curl == NULL || multiHandle == NULL) {
		return INTERNAL_ERROR;
	}

	/* Clear old data */
	this->rawData.clear();
	this->tweets.clear();
	this->nTweets = 0;

	this->stillRunning = 0;
	this->tweetsAuthor = username;
	this->disp = display;

	curl_multi_add_handle(multiHandle, curl);

	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	/* Get *n_tweets* Tweets from *username* */
	std::string tweetsURL = TWITTER_API_QUERY;
	tweetsURL += "?";
	tweetsURL += TWITTER_API_NAME + this->tweetsAuthor;
	tweetsURL += "&";
	tweetsURL += TWITTER_API_COUNT + std::to_string(n_tweets);
	curl_easy_setopt(curl, CURLOPT_URL, tweetsURL.c_str());

	struct curl_slist* slist = NULL;
	slist = curl_slist_append(slist, std::string("Authorization: Bearer " + this->token).c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

	/* Callback */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlReadCallBack);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(this->rawData));

	this->curlMAns = curl_multi_perform(multiHandle, &(this->stillRunning));
	if (this->curlMAns != CURLM_OK) {
		return INTERNAL_ERROR;
	}

	this->downloading = true;

	printDownloadMsg();
	this->animationCLK = clock();

	return DOWNLOADING;
}
	
int TwitterLCD::getDownloadStatus() 
{
	if (!this->init) {
		return INTERNAL_ERROR;
	}

	this->curlMAns = curl_multi_perform(this->multiHandle, &(this->stillRunning));
	if (this->curlMAns != CURLM_OK) {
		curl_easy_cleanup(this->curl);
		return INTERNAL_ERROR;
	}
	else if (this->stillRunning) {
		/* curl has not finished yet. */
		//if (isgreater( (float)(clock() - this->animationCLK), ANIMATION_TIME_S*CLOCKS_PER_SEC)) {
			printDownloadMsg();
		//}

		return DOWNLOADING;
	}

	curl_multi_remove_handle(this->multiHandle, this->curl);
	curl_easy_cleanup(this->curl);
	curl_multi_cleanup(this->multiHandle);

	this->downloading = false;

	return FINISHED_OK;
}

int TwitterLCD::interruptDownload()
{
	this->downloading = false;

	curl_multi_remove_handle(this->multiHandle, this->curl);
	curl_easy_cleanup(this->curl);
	if (curl_multi_cleanup(this->multiHandle) != CURLM_OK) {
		return INTERNAL_ERROR;
	}

	return FINISHED_OK;
}

bool TwitterLCD::displayTweetsOnDisplay()
{
	if (!this->init) {
		return false;
	}

	if (this->downloading || this->disp == NULL) {
		return false;
	}

	try
	{
		nlohmann::json jsonParser = nlohmann::json::parse(this->rawData);
		for (auto element : jsonParser) {
			this->tweets.push_back(element["text"]);
			this->nTweets++;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		this->rawData.clear();
		return false;
	}

	this->rawData.clear();
	this->rawData.shrink_to_fit();	/* Save some memory */

	this->currentTweet = this->tweets.begin();
	this->animationTick = 0;
	this->animationCLK = 0;
	this->printTweet(*this->currentTweet);

	return true;
}

std::string TwitterLCD::getLatestTweet()
{
	return this->tweets.front();
}

bool TwitterLCD::setNextTweet()
{
	this->animationTick = 0;
	if (this->currentTweet == this->tweets.end()) {
		this->currentTweet = this->tweets.begin();
	}
	else {
		this->currentTweet++;
	}
	this->printTweet(*this->currentTweet);

	return true;
}

bool TwitterLCD::setPrevTweet()
{
	this->animationTick = 0;
	if (this->currentTweet == this->tweets.begin()) {
		this->currentTweet = this->tweets.end();
	}
	else {
		this->currentTweet--;
	}
	this->printTweet(*this->currentTweet);
	return true;
}

/********************* PRIVATE METHODS *********************/
bool TwitterLCD::getToken(void)
{
	if (this->downloading) {
		this->interruptDownload();
	}

	this->curl = curl_easy_init();
	if (curl == NULL) {
		return false;
	}

	CURLcode curlAns = CURLE_FAILED_INIT;
	nlohmann::json json_parser;

	this->rawData.clear();

	/* Twitter OAuth2 */
	curl_easy_setopt(curl, CURLOPT_URL, "https://api.twitter.com/oauth2/token");
	/* Accept redirections */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	/* HTTPS protocol */
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

	/* Twitter auth key */
	std::string twitter_auth_key;
	twitter_auth_key = TWITTER_API_KEY;
	twitter_auth_key += ":";
	twitter_auth_key += TWITTER_API_SECRET;

	curl_easy_setopt(curl, CURLOPT_USERPWD, twitter_auth_key.c_str());

	struct curl_slist* slist = NULL;
	slist = curl_slist_append(slist, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
	if (slist == NULL) {
		return false;
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

	std::string data = "grant_type=client_credentials";
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

	/* Callback */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlReadCallBack);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(this->rawData));

	/* Get token from Twitter */
	curlAns = curl_easy_perform(curl);

	if (curlAns != CURLE_OK) {
		return false;
	}

	/* Clean */
	curl_easy_cleanup(curl);

	/* Parse token from Twitter's answer */
	try {
		json_parser = nlohmann::json::parse(this->rawData);

		std::string aux = json_parser["access_token"];
		this->token = aux;

		this->rawData.clear();
	}
	catch (std::exception &e){
		std::cerr << e.what() << std::endl;

		this->rawData.clear();
		return false;
	}

	return true;
}


void TwitterLCD::printTweet(std::string content)
{
	std::string header = "@" + this->tweetsAuthor + " dijo:  ";
	printToDisplay(header, content, this->animationTick);

	/* Prevent possible overflow */
	if (animationTick > header.size()-1 && animationTick > content.size()-1) {
		animationTick = 0;
	}
}
void TwitterLCD::printDownloadMsg()
{
	std::string header = "Bajando tweets de @" + this->tweetsAuthor + "  ";
	std::string progressBar = "----****----****";
	printToDisplay(header, progressBar, this->animationTick);

	/* Prevent possible overflow */
	if (animationTick > header.size()-1) {
		animationTick = 0;
	}
}

void TwitterLCD::printToDisplay(std::string topRow, std::string bottomRow, unsigned& animationCounter)
{
	if (this->disp == NULL)
		return;

	basicLCD& display = *(this->disp);
	if (!display.lcdInitOk())
		return;

	const cursorPosition pos = { 0, 0 };
	
	display.lcdClear();
	display.lcdSetCursorPosition(pos);

	topRow = this->stringRing(animationCounter, topRow);
	bottomRow = this->stringRing(animationCounter, bottomRow);

	if (topRow.size() > LCD_COLS) {
		topRow.erase(LCD_COLS);
	}
	else if (topRow.size() < LCD_COLS) {
		topRow.append(LCD_COLS - topRow.size(), ' ');
	}
	if (bottomRow.size() > LCD_COLS) {
		bottomRow.erase(LCD_COLS);
	}
	else if (bottomRow.size() < LCD_COLS) {
		bottomRow.append(LCD_COLS - bottomRow.size(), ' ');
	}

	display << topRow.c_str() << bottomRow.c_str();

	animationCounter++;
}

std::string TwitterLCD::stringRing(unsigned pos, std::string original) 
{
	if (pos == 0)
		return original;

	if (pos > original.size()) {
		pos = pos % original.size();
	}
	std::string beforePos = original.substr(0, pos);
	std::string afterPos = original.substr(pos);

	return std::string(afterPos + beforePos);
}
/********************* STATIC METHODS *********************/
static size_t curlReadCallBack(void* data, size_t size, size_t nmemb, void* userp)
{
	if (data == NULL || userp == NULL) {
		return 0;
	}

	size_t realsize = size * nmemb;
	std::string *  str = (std::string *)userp;

	str->append((char*)data, realsize);

	return realsize;
}
