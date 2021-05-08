#pragma once
#ifndef _TWITTERLCD_H_
#define _TWITTERLCD_H_  1

#include <ctime>			/* clock_t */
#include <list>
#include <string>

#include <curl/curl.h>

#include <LCD/LCD.h>

enum TWEETS_STATUS {
	INTERNAL_ERROR = -2,
	INVALID_ARGS = -1,
	FINISHED_OK = 0,
	DOWNLOADING,
	COULD_NOT_CONNECT,
	USER_IS_PRIVATE,
	USER_NOT_EXISTS,
	USER_HAS_NO_TWEETS,
	USER_HAS_LESS_TWEETS,   /* Requested N tweets but user has M tweets. With N > M */
	TWITTER_ERROR,
};

class TwitterLCD
{
public:
	TwitterLCD();
	~TwitterLCD();

	bool isInit();

	/* Returns code using TWEETS_STATUS. If everything is OK, you should get 'DOWNLOADING' */
	int startDownloading(std::string username, basicLCD* display, unsigned long long n_tweets = 50);
	
	/* Returns code using TWEETS_STATUS. */
	int getDownloadStatus();
	int interruptDownload();
	
	/* Call this once after downloading has finished */
	bool displayTweetsOnDisplay(); 

	/* Puts next tweet on the display. Does nothing if already in last. */
	bool setNextTweet();
	/* Puts previous tweet on the display. Does nothing if already in newest. */
	bool setPrevTweet();

	std::string getLatestTweet();

private:
	bool init;

	unsigned long long nTweets;                /* Number of actually downloaded tweets */
	std::string tweetsAuthor;
	std::list<std::string> tweets;
	std::list<std::string>::const_iterator currentTweet;

	basicLCD * disp;

	/* curl */
	CURL* curl;
	CURLM* multiHandle;
	CURLMcode curlMAns;

	bool downloading;
	int stillRunning;
	

	/* Twitter */
	std::string rawData;			/* JSON */
	std::string token;				/* Twitter */

	/* Display animation */
	clock_t animationCLK;
	unsigned animationTick;

	bool getToken();
	void printDownloadMsg();
	void printTweet(std::string tweet);

	void printToDisplay(std::string topRow, std::string bottomRow, unsigned& animationCounter);
	std::string stringRing(unsigned pos, std::string original);

};

#endif /* ! _TWITTERLCD_H_ */
