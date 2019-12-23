
#pragma once
#include <string>
#include <sstream>
#include "../include/AddInDefBase.h"

class Utils
{
public:
	static std::string wsToString(const std::wstring ws);
	template<typename T> static std::string anyToString(const T& t);
};


template<typename T> std::string Utils::anyToString(const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}