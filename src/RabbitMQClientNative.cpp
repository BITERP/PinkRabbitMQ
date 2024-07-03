
#if defined( __linux__ ) || defined(__APPLE__)

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <iconv.h>
#include <sys/time.h>

#endif

#include <stdio.h>
#include <wchar.h>
#include "RabbitMQClientNative.h"
#include <string>

Biterp::Names RabbitMQClientNative::properties{{
	{RabbitMQClientNative::ePropVersion, {u"Version"}},
	{RabbitMQClientNative::ePropCorrelationId, {u"CorrelationId"}},
	{RabbitMQClientNative::ePropType, {u"Type"}},
	{RabbitMQClientNative::ePropMessageId, {u"MessageId"}},
	{RabbitMQClientNative::ePropAppId, {u"AppId"}},
	{RabbitMQClientNative::ePropContentEncoding, {u"ContentEncoding"}},
	{RabbitMQClientNative::ePropContentType, {u"ContentType"}},
	{RabbitMQClientNative::ePropUserId, {u"UserId"}},
	{RabbitMQClientNative::ePropClusterId, {u"ClusterId"}},
	{RabbitMQClientNative::ePropExpiration, {u"Expiration"}},
	{RabbitMQClientNative::ePropReplyTo, {u"ReplyTo"}},
}};

Biterp::Names RabbitMQClientNative::methods{{
	{RabbitMQClientNative::eMethGetLastError, {u"GetLastError"}},
	{RabbitMQClientNative::eMethConnect, {u"Connect"}},
	{RabbitMQClientNative::eMethDeclareQueue, {u"DeclareQueue"}},
	{RabbitMQClientNative::eMethBasicPublish, {u"BasicPublish"}},
	{RabbitMQClientNative::eMethBasicConsume, {u"BasicConsume"}},
	{RabbitMQClientNative::eMethBasicConsumeMessage, {u"BasicConsumeMessage"}},
	{RabbitMQClientNative::eMethBasicCancel, {u"BasicCancel"}},
	{RabbitMQClientNative::eMethBasicAck, {u"BasicAck"}},
	{RabbitMQClientNative::eMethDeleteQueue, {u"DeleteQueue"}},
	{RabbitMQClientNative::eMethBindQueue, {u"BindQueue"}},
	{RabbitMQClientNative::eMethBasicReject, {u"BasicReject"}},
	{RabbitMQClientNative::eMethDeclareExchange, {u"DeclareExchange"}},
	{RabbitMQClientNative::eMethDeleteExchange, {u"DeleteExchange"}},
	{RabbitMQClientNative::eMethUnbindQueue, {u"UnbindQueue"}},
	{RabbitMQClientNative::eMethSetPriority, {u"SetPriority"}},
	{RabbitMQClientNative::eMethGetPriority, {u"GetPriority"}},
	{RabbitMQClientNative::eMethGetRoutingKey, {u"GetRoutingKey"}},
	{RabbitMQClientNative::eMethGetHeaders, {u"GetHeaders"}},
	{RabbitMQClientNative::eMethSleepNative, {u"SleepNative"}},
}};


const char16_t* RabbitMQClientNative::componentName = u"PinkRabbitMQ" QUOTE(NAME_POSTFIX);

// CAddInNative
//---------------------------------------------------------------------------//
RabbitMQClientNative::RabbitMQClientNative() {
	impl.LOGD("construct");
}

//---------------------------------------------------------------------------//
RabbitMQClientNative::~RabbitMQClientNative() {
	impl.LOGD("destruct");
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::Init(VOID_PTR pConnection) {
	impl.LOGD("init start");
	bool ret = impl.init(static_cast<IAddInDefBase*>(pConnection));
	impl.LOGD("init end");
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
	impl.LOGD("done start");
	impl.done();
	impl.LOGD("done end");
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
	return properties.size();//ePropLast;
}

//---------------------------------------------------------------------------//
long RabbitMQClientNative::FindProp(const WCHAR_T* wsPropName) {
	long plPropNum = properties.find((char16_t*)wsPropName);
	if (plPropNum == -1)
		impl.setLastError(u"Property not found: " + std::u16string((char16_t*)wsPropName));

	return plPropNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* RabbitMQClientNative::GetPropName(long lPropNum, long lPropAlias) {
	const std::u16string& name = properties.name(lPropNum, lPropAlias);
	if (name.empty()){
		return NULL;
	}
	return (WCHAR_T*)impl.memoryManager().allocString(name.c_str());

}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal) {
	impl.LOGD("1C get prop start " + properties.utf8(lPropNum));
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
	impl.LOGD("1C get prop end " + properties.utf8(lPropNum));
	return ret;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::SetPropVal(const long lPropNum, tVariant* varPropVal) {
	impl.LOGD("1C set prop start " + properties.utf8(lPropNum));
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
	impl.LOGD("1C set prop end " + properties.utf8(lPropNum));
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
	long plMethodNum = methods.find((char16_t*)wsMethodName);
	if (plMethodNum == -1)
		impl.setLastError(u"Method not found: " + std::u16string((char16_t*)wsMethodName));

	return plMethodNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* RabbitMQClientNative::GetMethodName(const long lMethodNum, const long lMethodAlias) {
	const std::u16string& name = methods.name(lMethodNum, lMethodAlias);
	if (name.empty()){
		return NULL;
	}
	return (WCHAR_T*)impl.memoryManager().allocString(name.c_str());
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
	impl.LOGD("1C call proc start " + methods.utf8(lMethodNum));
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
	impl.LOGD("1C call proc end " + methods.utf8(lMethodNum));
	return ret;
}

//---------------------------------------------------------------------------//
bool RabbitMQClientNative::CallAsFunc(const long lMethodNum,
	tVariant* pvarRetValue, tVariant* paParams,
	const long lSizeArray) {
	impl.LOGD("1C call func start " + methods.utf8(lMethodNum));
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
	impl.LOGD("1C call func end " + methods.utf8(lMethodNum));
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

