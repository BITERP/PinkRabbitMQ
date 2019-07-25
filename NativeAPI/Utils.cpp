
#include "Utils.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <string>

void Utils::convetToWChar(wchar_t* buffer, const char* text)
{
	size_t size = strlen(text) + 1;

	size_t outSize;
	mbstowcs_s(&outSize, buffer, size, text, size - 1);
};

std::wstring Utils::stringToWs(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.from_bytes(s);
}

std::string Utils::wsToString(const std::wstring& ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(ws);
}

wchar_t* Utils::wstring2wchar(std::wstring source) {
	wchar_t* tempWide = const_cast<wchar_t*>(source.c_str());
	return tempWide;
}