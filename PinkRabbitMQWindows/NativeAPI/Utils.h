
#pragma once
#include <types.h>
#include <string>
#include <sstream>

class Utils
{
public:
	static void convetToWChar(wchar_t* buffer, const char* text);
	static std::wstring stringToWs(const std::string& s);
	static std::string wsToString(const std::wstring& ws);
	static wchar_t* wstring2wchar(std::wstring source);
	template<typename T> static std::string anyToString(const T& t);
};

template<typename T> std::string Utils::anyToString(const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}
