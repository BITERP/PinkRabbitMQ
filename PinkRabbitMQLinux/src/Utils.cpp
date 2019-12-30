
#include "Utils.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <string>
#include <stdlib.h>
#include <locale>

std::string Utils::wsToString(const std::wstring &ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(ws);
}

std::wstring Utils::stringToWs(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.from_bytes(s);
}

char* Utils::stringToChar(const std::string &str) {
	char* writable = new char[str.size() + 1];
	std::copy(str.begin(), str.end(), writable);
	writable[str.size()] = '\0'; // don't forget the terminating 0
	return writable;
}

wchar_t* Utils::wstringToWchar(std::wstring &source) {
	return &source[0];
}