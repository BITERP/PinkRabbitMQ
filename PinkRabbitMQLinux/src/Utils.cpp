
#include "Utils.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <string>
#include <stdlib.h>
#include <locale>

std::string Utils::wsToString(const std::wstring ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(ws);
}