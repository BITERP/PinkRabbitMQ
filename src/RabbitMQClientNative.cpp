
#if defined( __linux__ ) || defined(__APPLE__)

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <iconv.h>
#include <sys/time.h>

#endif

#include <stdio.h>
#include <wchar.h>
#include "RabbitMQClientNative.h"
#include <string>
#include <codecvt>

static const char16_t* g_PropNames[] = {
	u"Version",
	u"CorrelationId",
	u"Type",
	u"MessageId",
	u"AppId",
	u"ContentEncoding",
	u"ContentType",
	u"UserId",
	u"ClusterId",
	u"Expiration",
	u"ReplyTo",
};
static const char16_t* g_MethodNames[] = {
	u"GetLastError",
	u"Connect",
	u"DeclareQueue",
	u"BasicPublish",
	u"BasicConsume",
	u"BasicConsumeMessage",
	u"BasicCancel",
	u"BasicAck",
	u"DeleteQueue",
	u"BindQueue",
	u"BasicReject",
	u"DeclareExchange",
	u"DeleteExchange",
	u"UnbindQueue",
	u"SetPriority",
	u"GetPriority",
	u"GetRoutingKey",
	u"GetHeaders",
	u"SleepNative",
};

static const char16_t* g_PropNamesRu[] = {
	u"Version",
	u"CorrelationId",
	u"Type",
	u"MessageId",
	u"AppId",
	u"ContentEncoding",
	u"ContentType",
	u"UserId",
	u"ClusterId",
	u"Expiration",
	u"ReplyTo",
};
static const char16_t* g_MethodNamesRu[] = {
	u"GetLastError",
	u"Connect",
	u"DeclareQueue",
	u"BasicPublish",
	u"BasicConsume",
	u"BasicConsumeMessage",
	u"BasicCancel",
	u"BasicAck",
	u"DeleteQueue",
	u"BindQueue",
	u"BasicReject",
	u"DeclareExchange",
	u"DeleteExchange",
	u"UnbindQueue",
	u"SetPriority",
	u"GetPriority",
	u"GetRoutingKey",
	u"GetHeaders",
	u"SleepNative",
};

const char16_t* RabbitMQClientNative::componentName = u"PinkRabbitMQ";

// CAddInNative
//---------------------------------------------------------------------------//
RabbitMQClientNative::RabbitMQClientNative() {
	LOGD("construct");
}

//---------------------------------------------------------------------------//
RabbitMQClientNative::~RabbitMQClientNative() {
	LOGD("destruct");
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::Init(VOID_PTR pConnection) {
	LOGD("init start");
	bool ret = impl.init(static_cast<IAddInDefBase*>(pConnection));
	LOGD("init end");
	return ret;
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::GetInfo() {
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}

//---------------------------------------------------------------------------//
void RabbitMQClientNative::Done() {
	LOGD("done start");
	impl.done();
	LOGD("done end");
}

/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool RabbitMQClientNative::RegisterExtensionAs(WCHAR_T** wsExtensionName) {
	return impl.memoryManager().copyString((char16_t**)wsExtensionName, componentName);
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::GetNProps() {
	// You may delete next lines and add your own implementation code here
	return ePropLast;
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::FindProp(const WCHAR_T* wsPropName) {
	long plPropNum = -1;
	plPropNum = findName(g_PropNames, (char16_t*)wsPropName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, (char16_t*)wsPropName, ePropLast);

	if (plPropNum == -1)
		impl.setLastError(u"Property not found: " + u16string((char16_t*)wsPropName));

	return plPropNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* RabbitMQClientNative::GetPropName(long lPropNum, long lPropAlias) {
	if (lPropNum >= ePropLast)
		return NULL;

	const char16_t* wsCurrentName = NULL;

	switch (lPropAlias) {
	case 0: // First language
		wsCurrentName = g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	return (WCHAR_T*)impl.memoryManager().allocString(wsCurrentName);
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal) {
	LOGD("1C get prop start " + to_string(lPropNum));
	bool ret = false;
	switch (lPropNum) {
	case ePropVersion:
		ret = impl.getVersion(pvarPropVal);
		break;
	case ePropCorrelationId:
	case ePropType:
	case ePropMessageId:
	case ePropAppId:
	case ePropContentEncoding:
	case ePropContentType:
	case ePropUserId:
	case ePropClusterId:
	case ePropExpiration:
	case ePropReplyTo:
		ret = impl.getMsgProp(pvarPropVal, lPropNum);
		break;
	default:
		ret = false;
		break;
	}
	LOGD("1C get prop end " + to_string(lPropNum));
	return ret;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::SetPropVal(const long lPropNum, tVariant* varPropVal) {
	LOGD("1C set prop start " + to_string(lPropNum));
	bool ret = false;
	switch (lPropNum) {
	case ePropCorrelationId:
	case ePropType:
	case ePropMessageId:
	case ePropAppId:
	case ePropContentEncoding:
	case ePropContentType:
	case ePropUserId:
	case ePropClusterId:
	case ePropExpiration:
	case ePropReplyTo:
		ret = impl.setMsgProp(varPropVal, lPropNum);
		break;
	default:
		ret = false;
		break;
	}
	LOGD("1C set prop end " + to_string(lPropNum));
	return ret;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::IsPropReadable(const long /*lPropNum*/) {
	return true;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::IsPropWritable(const long lPropNum) {
	return lPropNum >= ePropCorrelationId && lPropNum < ePropLast;
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::GetNMethods() {
	return eMethLast;
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::FindMethod(const WCHAR_T* wsMethodName) {
	long plMethodNum = -1;
	plMethodNum = findName(g_MethodNames, (char16_t*)wsMethodName, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, (char16_t*)wsMethodName, eMethLast);

	if (plMethodNum == -1)
		impl.setLastError(u"Method not found: " + u16string((char16_t*)wsMethodName));

	return plMethodNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* RabbitMQClientNative::GetMethodName(const long lMethodNum, const long lMethodAlias) {
	if (lMethodNum >= eMethLast)
		return NULL;

	const char16_t* wsCurrentName = NULL;

	switch (lMethodAlias) {
	case 0: // First language
		wsCurrentName = g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}
	return (WCHAR_T*)impl.memoryManager().allocString(wsCurrentName);
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::GetNParams(const long lMethodNum) {
	switch (lMethodNum)
	{
	case eMethConnect:
		return 8;
	case eMethDeclareQueue:
		return 7;
	case eMethBasicPublish:
	case eMethDeclareExchange:
	case eMethBasicConsume:
		return 6;
	case eMethBasicConsumeMessage:
	case eMethBindQueue:
		return 4;
	case eMethDeleteQueue:
	case eMethUnbindQueue:
		return 3;
	case eMethDeleteExchange:
		return 2;
	case eMethBasicCancel:
	case eMethBasicAck:
	case eMethBasicReject:
	case eMethSetPriority:
	case eMethSleepNative:
		return 1;
	default:
		return 0;
	}
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
	tVariant* pvarParamDefValue) {
	switch (lMethodNum)
	{
	case eMethConnect:
		if (lParamNum == 5) {
			TV_VT(pvarParamDefValue) = VTYPE_I4;
			TV_I4(pvarParamDefValue) = 0;
			return true;
		}
		if (lParamNum == 6) {
			TV_VT(pvarParamDefValue) = VTYPE_BOOL;
			TV_BOOL(pvarParamDefValue) = false;
			return true;
		}
		if (lParamNum == 7) {
			TV_VT(pvarParamDefValue) = VTYPE_I4;
			TV_I4(pvarParamDefValue) = 5;
			return true;
		}
		break;
	case eMethDeclareQueue:
		if (lParamNum == 5) {
			TV_VT(pvarParamDefValue) = VTYPE_I4;
			TV_I4(pvarParamDefValue) = 0;
			return true;
		}
		if (lParamNum == 6) {
			TV_VT(pvarParamDefValue) = VTYPE_PWSTR;
			TV_WSTR(pvarParamDefValue) = nullptr;
			pvarParamDefValue->wstrLen = 0;
			return true;
		}
		break;
	case eMethBasicPublish:
	case eMethDeclareExchange:
	case eMethBasicConsume:
		if (lParamNum == 5) {
			TV_VT(pvarParamDefValue) = VTYPE_PWSTR;
			TV_WSTR(pvarParamDefValue) = nullptr;
			pvarParamDefValue->wstrLen = 0;
			return true;
		}
		break;
	case eMethBindQueue:
		if (lParamNum == 3) {
			TV_VT(pvarParamDefValue) = VTYPE_PWSTR;
			TV_WSTR(pvarParamDefValue) = nullptr;
			pvarParamDefValue->wstrLen = 0;
			return true;
		}
		break;
	}
	return false;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::HasRetVal(const long lMethodNum) {
	switch (lMethodNum)
	{
	case eMethGetLastError:
	case eMethBasicConsume:
	case eMethBasicConsumeMessage:
	case eMethDeclareQueue:
	case eMethGetPriority:
	case eMethGetRoutingKey:
	case eMethGetHeaders:
		return true;
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::CallAsProc(const long lMethodNum,
	tVariant* paParams, const long lSizeArray) {
	LOGD("1C call proc start " + to_string(lMethodNum));
	bool ret = false;
	switch (lMethodNum) {
	case eMethConnect:
		ret = impl.connect(paParams, lSizeArray);
		break;
	case eMethBasicPublish:
		ret = impl.basicPublish(paParams, lSizeArray);
		break;
	case eMethBasicCancel:
		ret = impl.basicCancel(paParams, lSizeArray);
		break;
	case eMethBasicAck:
		ret = impl.basicAck(paParams, lSizeArray);
		break;
	case eMethBasicReject:
		ret = impl.basicReject(paParams, lSizeArray);
		break;
	case eMethDeleteQueue:
		ret = impl.deleteQueue(paParams, lSizeArray);
		break;
	case eMethBindQueue:
		ret = impl.bindQueue(paParams, lSizeArray);
		break;
	case eMethUnbindQueue:
		ret = impl.unbindQueue(paParams, lSizeArray);
		break;
	case eMethDeclareExchange:
		ret = impl.declareExchange(paParams, lSizeArray);
		break;
	case eMethDeleteExchange:
		ret = impl.deleteExchange(paParams, lSizeArray);
		break;
	case eMethSetPriority:
		ret = impl.setPriority(paParams, lSizeArray);
		break;
	case eMethSleepNative:
		ret = impl.sleepNative(paParams, lSizeArray);
		break;
	default:
		ret = false;
		break;
	}
	LOGD("1C call proc end " + to_string(lMethodNum));
	return ret;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::CallAsFunc(const long lMethodNum,
	tVariant* pvarRetValue, tVariant* paParams,
	const long lSizeArray) {
	LOGD("1C call func start " + to_string(lMethodNum));
	bool ret = false;
	switch (lMethodNum) {
	case eMethGetLastError:
		ret = impl.getLastError(pvarRetValue, paParams, lSizeArray);
		break;
	case eMethBasicConsume:
		ret = impl.basicConsume(pvarRetValue, paParams, lSizeArray);
		break;
	case eMethBasicConsumeMessage:
		ret = impl.basicConsumeMessage(pvarRetValue, paParams, lSizeArray);
		break;
	case eMethDeclareQueue:
		ret = impl.declareQueue(pvarRetValue, paParams, lSizeArray);
		break;
	case eMethGetPriority:
		ret = impl.getPriority(pvarRetValue, paParams, lSizeArray);
		break;
	case eMethGetRoutingKey:
		ret = impl.getRoutingKey(pvarRetValue, paParams, lSizeArray);
		break;
	case eMethGetHeaders:
		ret = impl.getHeaders(pvarRetValue, paParams, lSizeArray);
		break;
	default:
		ret = false;
		break;
	}
	LOGD("1C call func end " + to_string(lMethodNum));
	return ret;
}


//---------------------------------------------------------------------------//
void RabbitMQClientNative::SetLocale(const WCHAR_T* loc) {
#if !defined( __linux__ ) && !defined(__APPLE__)
	_wsetlocale(LC_ALL, (wchar_t*)loc);
#else
	//We convert in char* char_locale
	//also we establish locale
	//setlocale(LC_ALL, char_locale);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool RabbitMQClientNative::setMemManager(void* mem) {
	impl.memoryManager().setHandle((IMemoryManager*)mem);
	return mem != 0;
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::findName(const char16_t* names[], u16string name, const uint32_t size) const {
	long ret = -1;
	for (uint32_t i = 0; i < size; i++) {
		if (name == names[i]) {
			ret = i;
			break;
		}
	}
	return ret;
}
