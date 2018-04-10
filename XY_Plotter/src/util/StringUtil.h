/*
 * StringUtil.h
 *
 *  Created on: Aug 31, 2017
 *      Author: robin
 */

#ifndef UTIL_STRINGUTIL_H_
#define UTIL_STRINGUTIL_H_

#include <string>
#include <queue>

class StringUtil {
public:
	StringUtil();
	virtual ~StringUtil();
	static std::queue<std::string> split(std::string& line, std::string delim);

};

#endif /* UTIL_STRINGUTIL_H_ */
