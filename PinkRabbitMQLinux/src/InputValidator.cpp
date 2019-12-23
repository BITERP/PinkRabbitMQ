#include "InputValidator.h"
#include "Utils.h"
#include "AddInNative.h"

bool InputValidator::checkInputParameter(tVariant* params, long const methodNum, long const parameterNum, ENUMVAR type) {
	if (!(TV_VT(&params[parameterNum]) == type)) {
		std::string errDescr = "Error occured when calling method "
			+ Utils::wsToString(GetMethodName(methodNum, 1))
			+ "() - wrong type for parameter number "
			+ Utils::anyToString(parameterNum);

		addError(ADDIN_E_FAIL, L"NativeRabbitMQ", Utils::stringToWs(errDescr).c_str(), 1);
		client->updateLastError(errDescr.c_str());
		return false;
	}
	return true;
}