#pragma once
#ifndef _TWITTERLCD_H_
#define _TWITTERLCD_H_  1

#include <ctime>			/* clock_t */
#include <list>
#include <string>

#include <curl/curl.h>

#include <LCD/LCD.h>

enum TWEETS_STATE {
	INTERNAL_ERROR = -2,
	INVALID_ARGS = -1,
	FINISHED_OK = 0,
	TWEETS_OK = 0,
	DOWNLOADING,
	COULD_NOT_CONNECT,
	USER_IS_PRIVATE,
	USER_NOT_EXISTS,
	USER_HAS_NO_TWEETS,
	USER_HAS_LESS_TWEETS,   /* Requested N tweets but user has M tweets. With N > M */
	TWITTER_ERROR,
};

typedef struct {
	std::string date;
	std::string text;
	std::string time;
} _tweet_t;

class TwitterLCD
{
public:
	TwitterLCD();
	~TwitterLCD();

	bool isInit();

	/* After downloading, returns accordingly to TWEETS_STATE.
	* If a tweets download is in progress, it returns DOWNLOADING
	* If this class is not initialized, returns INVALID_ARGS
	*/
	int getState();

	/* Returns code using TWEETS_STATE. If everything is OK, you should get 'DOWNLOADING' */
	bool startDownloading(std::string username, basicLCD* display, unsigned long long n_tweets = 50);
	
	/* Call this whenever you want to update whatever the display is showing */
	bool updateDisplay();

	/* Tweets have been downloaded and are being shown on display */
	bool hasTweets();
	/* Number of tweets downloaded and ready to show. */
	unsigned long long tweetsAvailable();
	/* Number of tweets that where intended to download. 
	* It is not always equal to tweetsAvailable
	*/
	unsigned long long tweetsRequested();

	/* Returns code using TWEETS_STATE. */
	bool getDownloadState();	/* True if still downloading. False otherwise */
	bool interruptDownload();
	
	/* Call this once after downloading has finished */
	bool showTweetsOnDisplay(); 

	/* Puts next tweet on the display. Does nothing if already in last. */
	bool setNextTweet();
	/* Puts previous tweet on the display. Does nothing if already in newest. */
	bool setPrevTweet();

	std::string getLatestTweet();

private:
	bool init;
	int errorCode;

	unsigned long long nTweets;                /* Number of actually downloaded tweets */
	std::string tweetsAuthor;
	std::list<_tweet_t> tweets;
	std::list<_tweet_t>::const_iterator currentTweet;

	basicLCD * disp;

	/* curl */
	CURL* curl;
	CURLM* multiHandle;
	CURLMcode curlMAns;

	bool downloading;
	int stillRunning;
	unsigned long long askedTweets;
	

	/* Twitter */
	std::string rawData;			/* JSON */
	std::string token;				/* Twitter */

	/* Display animation */
	clock_t animationCLK;
	unsigned animationTick;

	bool getToken();

	/* Get Twitter API Key from apikey.json file */
	bool getApiKey(std::string& key, std::string& secret);

	void printDownloadMsg();
	void printTweet(_tweet_t tweet);

	void printToDisplay(std::string topRow, std::string bottomRow, unsigned& animationCounter);
	std::string stringRing(unsigned pos, std::string original);


	int validTweets(void* parsedJsonData);
};

#endif /* ! _TWITTERLCD_H_ */
