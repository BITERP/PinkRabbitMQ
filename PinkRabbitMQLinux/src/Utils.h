
#pragma once
#include <string>
#include <sstream>
#include "../include/AddInDefBase.h"

class Utils
{
public:
	static std::string wsToString(const std::wstring ws);
	static std::wstring stringToWs(const std::string& s);
	template<typename T> static std::string anyToString(const T& t);
	static char* stringToChar(std::string str);
	static wchar_t* wstringToWchar(std::wstring source);
};


template<typename T> std::string Utils::anyToString(const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}