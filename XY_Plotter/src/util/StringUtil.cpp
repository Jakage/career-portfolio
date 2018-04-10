/*
 * StringUtil.cpp
 *
 *  Created on: Aug 31, 2017
 *      Author: robin
 */

#include "StringUtil.h"

StringUtil::StringUtil() {

}

StringUtil::~StringUtil() {
}

// Taken from: https://stackoverflow.com/a/37454181
std::queue<std::string> StringUtil::split(std::string& line, std::string delim) {
	std::queue<std::string> output;
	size_t prev = 0, pos = 0;

	do {
		pos = line.find(delim, prev);
		if(pos == std::string::npos) {
			pos = line.size();
		}

		output.push(line.substr(prev, pos - prev));
		prev = pos + delim.length();
	} while(pos < line.length() && prev < line.length());

	return output;
}


