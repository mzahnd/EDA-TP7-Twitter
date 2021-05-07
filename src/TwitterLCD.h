#pragma once
#ifndef _TWITTERLCD_H_
#define _TWITTERLCD_H_  1

#include <list>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

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
};

#endif /* ! _TWITTERLCD_H_ */
