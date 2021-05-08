#include <iostream>

#include "DisplayMartin.hpp"
#include "TwitterLCD.h"

int main(void)
{
	DisplayMartin dispM;
	if (!dispM.lcdInitOk())
		return 1;

	TwitterLCD tweet;
	if (!tweet.isInit())
		return 1;

	int status = -3;
	status = tweet.startDownloading("foxnews", &dispM, 200);

	while (status == DOWNLOADING)
	{
		status = tweet.getDownloadStatus();
	}

	if (status != FINISHED_OK) {
		printf("Error getting tweets!\n");
		printf("Status code: %d\n", status);
	}
	else {
		tweet.displayTweetsOnDisplay();
		tweet.setNextTweet();

	}

	return 0;
}