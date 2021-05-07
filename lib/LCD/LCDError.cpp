#include "LCDError.h"

/********************* CONSTRUCTOR *********************/
LCDError::LCDError()
{
	errorCode = 0;
	name.clear();
	description.clear();
}

/********************* PUBLIC METHODS *********************/
std::string LCDError::getErrorName() {
	return this->name;
}

std::string LCDError::getErrorDescription() {
	return this->description;
}

unsigned long LCDError::getErrorCode() {
	return this->errorCode;
}

bool LCDError::setError(const char* name_, const char* description_, unsigned long int code_) {
	int success = true;
	if (name_ != NULL) {
		(this->name).clear();
		(this->name).append(name_);
		success &= true;
	} else {
		success &= false;
	}
	
	if (description_ != NULL) {
		(this->description).clear();
		(this->description).append(description_);
		success &= true;
	} else {
		success &= false;
	}

	if (code_ != 0) {
		this->errorCode = code_;
		success &= true;
	} else {
		success &= false;
	}
	
	return success;
}