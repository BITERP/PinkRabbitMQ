#pragma once
#include "Types.h"
class FieldValidators
{
	bool validateConnect(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBasicPublish(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateDeclDelQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBindUnbindQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateDeclareExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateDeleteExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBasicConsumeMessage(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBasicConsume(tVariant* paParams, long const lMethodNum, long const lSizeArray);
}