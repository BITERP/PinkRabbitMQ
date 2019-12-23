#pragma once
#include <types.h>
class InputValidator {
public:
	bool checkInputParameter(tVariant* params, long const methodNum, long const parameterNum, ENUMVAR type);
};