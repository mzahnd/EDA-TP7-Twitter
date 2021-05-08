#include <cmath>		/* isgreater */
#include <exception>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "TwitterLCD.h"

#define TWITTER_API_QUERY	"https://api.twitter.com/1.1/statuses/user_timeline.json"
#define TWITTER_API_NAME	"screen_name="
#define TWITTER_API_COUNT	"count="

#define LCD_COLS			(16)
#define ANIMATION_TIME_S	(1.0)

static size_t curlReadCallBack(void* data, size_t size, size_t nmemb, void* userp);

/********************* CONSTRUCTOR *********************/
TwitterLCD::TwitterLCD(void)
{
	this->init = true;

	this->errorCode = INTERNAL_ERROR;

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

	if (this->init) this->errorCode = TWEETS_OK;

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

bool TwitterLCD::hasTweets()
{
	if (this->nTweets == 0) return false;

	return true;
}

unsigned long long TwitterLCD::tweetsAvailable()
{
	return this->nTweets;
}

unsigned long long TwitterLCD::tweetsRequested()
{
	return this->askedTweets;
}

int TwitterLCD::getState() 
{
	if (!this->init)
		return INVALID_ARGS;

	if (this->downloading)
		return DOWNLOADING;

	return this->errorCode;
}

bool TwitterLCD::startDownloading(std::string username, basicLCD* display, unsigned long long n_tweets)
{
	if (!this->init) {
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	if (username.empty() || display == NULL || n_tweets == 0) {
		this->errorCode = INVALID_ARGS;
		return false;
	}

	this->curl = curl_easy_init();
	this->multiHandle = curl_multi_init();

	if (curl == NULL || multiHandle == NULL) {
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	/* Clear old data */
	this->rawData.clear();
	this->tweets.clear();
	this->nTweets = 0;

	this->stillRunning = 0;
	this->tweetsAuthor = username;
	this->askedTweets = n_tweets;
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
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	this->downloading = true;

	printDownloadMsg();
	this->animationCLK = clock();

	this->errorCode = DOWNLOADING;
	return true;
}

bool TwitterLCD::getDownloadState() 
{
	if (!this->init) {
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	if (!this->downloading) {
		this->errorCode = FINISHED_OK;
		return false;
	}

	this->curlMAns = curl_multi_perform(this->multiHandle, &(this->stillRunning));
	if (this->curlMAns != CURLM_OK) {
		curl_easy_cleanup(this->curl);
		this->errorCode = INTERNAL_ERROR;
		return false;
	}
	else if (this->stillRunning) {
		this->errorCode = DOWNLOADING;
		return true;
	}

	curl_multi_remove_handle(this->multiHandle, this->curl);
	curl_easy_cleanup(this->curl);
	curl_multi_cleanup(this->multiHandle);

	this->downloading = false;

	this->errorCode = FINISHED_OK;

	return false;
}

bool TwitterLCD::interruptDownload()
{
	this->downloading = false;

	curl_multi_remove_handle(this->multiHandle, this->curl);
	curl_easy_cleanup(this->curl);
	if (curl_multi_cleanup(this->multiHandle) != CURLM_OK) {
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	this->errorCode = FINISHED_OK;
	return true;
}

bool TwitterLCD::updateDisplay()
{
	if (this->disp == NULL) {
		this->errorCode = INVALID_ARGS;
		return false;
	}

	if (this->downloading) {
		printDownloadMsg();
	}
	else if (this->hasTweets()) {
		printTweet(*this->currentTweet);
	}
	else if (!this->hasTweets() && !this->tweetsAuthor.empty()) {
		std::string top = "@" + this->tweetsAuthor;
		std::string bottom = "Has no tweets";

		this->printToDisplay(top, bottom, this->animationTick);
		this->animationTick++;

		/* Avoid possible overflow */
		if (this->animationTick > top.size() && this->animationTick > bottom.size()) {
			this->animationTick = 0;
		}
	}
	else {
		std::string top = "Waiting...";
		std::string bottom = ":)";

		printToDisplay(top, bottom, this->animationTick);
		this->animationTick++;

		/* Avoid possible overflow */
		if (this->animationTick > top.size() && this->animationTick > bottom.size()) {
			this->animationTick = 0;
		}
	}
	return true;
}

bool TwitterLCD::showTweetsOnDisplay()
{
	if (!this->init) {
		return false;
	}

	if (this->downloading || this->disp == NULL) {
		return false;
	}

	nlohmann::json jsonParser;
	try
	{
		jsonParser = nlohmann::json::parse(this->rawData);
	
		int valid = this->validTweets(&jsonParser);
		if (valid != TWEETS_OK) {
			this->errorCode = valid;
			return false;
		}

		for (auto element : jsonParser) {
			_tweet_t current;

			/* created_at format*/
			// "Mon Nov 16 18:09:36 +0000 2020"
			std::string datetime = element["created_at"];
			std::string txt = element["text"];

			size_t time_init = datetime.find(':') - 2;
			size_t time_end = datetime.find('+') + 5;

			current.date = datetime.substr(0, time_init);
			current.date += datetime.substr(time_end+1); /* Remove extra space */

			current.time = datetime.substr(time_init, time_end-time_init);

			current.text = txt.substr(0, txt.find("http") - 2);
			current.text += "    ";

			this->tweets.push_back(current);
			this->nTweets++;
		}
	}
	catch (nlohmann::detail::parse_error& e) {
		this->errorCode = USER_HAS_LESS_TWEETS;

		if (this->nTweets == 0) {
			this->errorCode = USER_HAS_NO_TWEETS;
			this->rawData.clear();
			this->rawData.shrink_to_fit();	/* Save some memory */

			return false;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;

		this->rawData.clear();
		this->rawData.shrink_to_fit();	/* Save some memory */

		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	if (this->nTweets == 0) {
		this->errorCode = USER_HAS_NO_TWEETS;
	}
	else if (this->nTweets < this->askedTweets) {
		this->errorCode = USER_HAS_LESS_TWEETS;
	}
	else {
		this->errorCode = TWEETS_OK;
	}

	this->rawData.clear();
	this->rawData.shrink_to_fit();	/* Save some memory */

	this->currentTweet = this->tweets.begin();
	this->animationTick = 0;
	this->animationCLK = 0;

	this->updateDisplay();

	return true;
}

bool TwitterLCD::setNextTweet()
{
	if (!this->hasTweets()) return false;

	this->animationTick = 0;

	/* Last iterator is always invalid */
	std::list<_tweet_t>::const_iterator last = this->tweets.end();
	last--;

	if (this->currentTweet == last) {
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
	if (!this->hasTweets()) return false;

	this->animationTick = 0;
	if (this->currentTweet == this->tweets.begin()) {
		this->currentTweet = this->tweets.end();

		/* Last iterator is always invalid */
		this->currentTweet--;
	}
	else {
		this->currentTweet--;
	}
	this->printTweet(*this->currentTweet);
	return true;
}

/********************* PRIVATE METHODS *********************/
bool TwitterLCD::getApiKey(std::string& key, std::string& secret)
{
	std::ifstream file;
	std::string rawFileContent;
	nlohmann::json rawJson;

	file.open("apikey.json", std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	rawFileContent.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	try {
		rawJson = nlohmann::json::parse(rawFileContent);

		key = rawJson["twitter"]["key"];
		secret = rawJson["twitter"]["secret"];
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		key.clear();
		secret.clear();

		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	this->errorCode = FINISHED_OK;
	return true;
}

bool TwitterLCD::getToken(void)
{
	if (this->downloading) {
		this->interruptDownload();
	}

	this->curl = curl_easy_init();
	if (curl == NULL) {
		this->errorCode = INTERNAL_ERROR;
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
	std::string APIKey, APISecret;

	bool gotApi = getApiKey(APIKey, APISecret);
	if (!gotApi) {
		return false;
	}

	twitter_auth_key = APIKey + ":" + APISecret;

	curl_easy_setopt(curl, CURLOPT_USERPWD, twitter_auth_key.c_str());

	struct curl_slist* slist = NULL;
	slist = curl_slist_append(slist, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
	if (slist == NULL) {
		this->errorCode = INTERNAL_ERROR;
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
		switch (curlAns)
		{
		case CURLE_COULDNT_RESOLVE_HOST: /* Fall through*/
		case CURLE_COULDNT_CONNECT: 
		case CURLE_REMOTE_ACCESS_DENIED:
			this->errorCode = COULD_NOT_CONNECT;
			break;
		case CURLE_OUT_OF_MEMORY:
			fprintf(stderr, "Fsck.\n");
			this->errorCode = INTERNAL_ERROR;
			break;
		case CURLE_GOT_NOTHING:
			this->errorCode = TWITTER_ERROR;
			break;
		default:
			this->errorCode = INTERNAL_ERROR;
			break;
		}
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
		this->errorCode = FINISHED_OK;
	}
	catch (std::exception &e){
		std::cerr << e.what() << std::endl;

		this->rawData.clear();
		this->errorCode = INTERNAL_ERROR;
		return false;
	}

	return true;
}

void TwitterLCD::printTweet(_tweet_t content)
{
	std::string header = "El " + content.date + " a las " 
		+ content.time + " @" + this->tweetsAuthor + " dijo:    ";
	printToDisplay(header, content.text, this->animationTick);

	/* Prevent possible overflow */
	if (animationTick > header.size()-1 && animationTick > content.text.size()-1) {
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

	display.lcdClear();

	if (topRow.size() < LCD_COLS) {
		topRow.append(LCD_COLS - topRow.size(), ' ');
	}
	else {
		topRow = this->stringRing(animationCounter, topRow);
	}

	if (bottomRow.size() < LCD_COLS) {
		bottomRow.append(LCD_COLS - bottomRow.size(), ' ');
	}
	else {
		bottomRow = this->stringRing(animationCounter, bottomRow);
	}

	if (topRow.size() > LCD_COLS) {
		topRow.erase(LCD_COLS);
	}
	if (bottomRow.size() > LCD_COLS) {
		bottomRow.erase(LCD_COLS);
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

int TwitterLCD::validTweets(void* parsedJsonData)
{
	if (parsedJsonData == NULL) return INVALID_ARGS;

	nlohmann::json* j = (nlohmann::json*)parsedJsonData;
	int answer = TWEETS_OK;

	/* I don't really like this idea of exceptions :\ */
	try {
		std::string twitterError = j->at("error");

		std::cout << "twErr: " << twitterError << std::endl;
		if (twitterError == "Not authorized.") {
			answer = USER_IS_PRIVATE;
		}

	}
	catch (...) {
		answer = TWEETS_OK;
	}
	return answer;
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
