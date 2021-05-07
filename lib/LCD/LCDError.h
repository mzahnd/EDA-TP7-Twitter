#pragma once
#ifndef _LCDError_H_
#define _LCDError_H_	1

#include <string>

enum ERRORS { 
	ALINIT = 1,
	STRING_INIT, 
	DISPLAY,
	BACKGROUND,
	FONT,
	LEFT_BOUNDARY,
	RIGHT_BOUNDARY,
	UP_BOUNDARY,
	DOWN_BOUNDARY,
	WRITING_EXCEEDED, 
	OUT_OF_BOUNDS,
	FAIL_CLEAR
};

class LCDError
{
public:
	LCDError();
	
	std::string getErrorName();
	std::string getErrorDescription();
	unsigned long getErrorCode();

	bool setError(const char* name, const char* description, unsigned long int code);

private:
	std::string name;
	std::string description;
	unsigned long errorCode;
};

#endif /* ! _LCDError_H_ */