

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Examples for the report "Making external components for 1C mobile platform for Android""
// at the conference INFOSTART 2018 EVENT EDUCATION https://event.infostart.ru/2018/
//
// Sample 1: Delay in code
// Sample 2: Getting device information
// Sample 3: Device blocking: receiving external event about changing of sceen
//
// Copyright: Igor Kisil 2018
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "AddInNative.h"
#include "ConversionWchar.h"
#include "types.h"
#include "wchar.h"
#include <chrono>
#include <thread>
#include "Utils.h"

static const wchar_t *g_PropNames[] = 
{
	L"Version",
	L"CorrelationId",
	L"Type",
	L"MessageId",
	L"AppId",
	L"ContentEncoding",
	L"ContentType",
	L"UserId",
	L"ClusterId",
	L"Expiration",
	L"ReplyTo",
};

static const wchar_t* g_PropNamesRu[] =
{
	L"Version",
	L"CorrelationId",
	L"Type",
	L"MessageId",
	L"AppId",
	L"ContentEncoding",
	L"ContentType",
	L"UserId",
	L"ClusterId",
	L"Expiration",
	L"ReplyTo",
};

static const wchar_t *g_MethodNames[] =
{
	L"GetLastError",
	L"Connect",
	L"DeclareQueue",
	L"BasicPublish",
	L"BasicConsume",
	L"BasicConsumeMessage",
	L"BasicCancel",
	L"BasicAck",
	L"DeleteQueue",
	L"BindQueue",
	L"BasicReject",
	L"DeclareExchange",
	L"DeleteExchange",
	L"UnbindQueue",
	L"SetPriority",
	L"GetPriority",
};


static const wchar_t *g_MethodNamesRu[] =
{
	L"GetLastError",
	L"Connect",
	L"DeclareQueue",
	L"BasicPublish",
	L"BasicConsume",
	L"BasicConsumeMessage",
	L"BasicCancel",
	L"BasicAck",
	L"DeleteQueue",
	L"BindQueue",
	L"BasicReject",
	L"DeclareExchange",
	L"DeleteExchange",
	L"UnbindQueue",
	L"SetPriority",
	L"GetPriority",
};

static const wchar_t g_ComponentNameAddIn[] = L"PinkRabbitMQ";
static WcharWrapper s_ComponentClass(g_ComponentNameAddIn);
// This component supports 2.1 version
const long g_VersionAddIn = 2100;
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;

//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface)
	{
		*pInterface = new AddInNative();
		return (long)*pInterface;
	}
	return 0;
}

//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}

//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pInterface)
{
	if (!*pInterface)
		return -1;

	delete *pInterface;
	*pInterface = 0;
	return 0;
}

//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
	return s_ComponentClass;
}

AddInNative::AddInNative() : m_iConnect(nullptr), m_iMemory(nullptr)
{
}

AddInNative::~AddInNative()
{
}

/////////////////////////////////////////////////////////////////////////////
// IInitDoneBase
//---------------------------------------------------------------------------//
bool AddInNative::Init(void* pConnection)
{
	OPENSSL_init_ssl(0, NULL);

	m_iConnect = (IAddInDefBaseEx*)pConnection;
	if (m_iConnect)
	{
		return true;
	}
	return m_iConnect != nullptr;
}

//---------------------------------------------------------------------------//
bool AddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != nullptr;
}

//---------------------------------------------------------------------------//
long AddInNative::GetInfo()
{
	return g_VersionAddIn;
}

//---------------------------------------------------------------------------//
void AddInNative::Done()
{
	m_iConnect = nullptr;
	m_iMemory = nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool AddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	const wchar_t *wsExtension = g_ComponentNameAddIn;
	uint32_t iActualSize = static_cast<uint32_t>(::wcslen(wsExtension) + 1);

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
		{
			convToShortWchar(wsExtensionName, wsExtension, iActualSize);
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------//
long AddInNative::GetNProps()
{
	// You may delete next lines and add your own implementation code here
	return ePropLast;
	return ePropLast;
}

//---------------------------------------------------------------------------//
long AddInNative::FindProp(const WCHAR_T* wsPropName)
{
	long plPropNum = -1;
	wchar_t* propName = 0;
	convFromShortWchar(&propName, wsPropName);

	plPropNum = findName(g_PropNames, propName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);

	delete[] propName;
	return plPropNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* AddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsPropName = NULL;

	switch (lPropAlias)
	{
	case 0: // First language (english)
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language (local)
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	uint32_t iActualSize = static_cast<uint32_t>(wcslen(wsCurrentName) + 1);

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
			convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}

//---------------------------------------------------------------------------//
bool AddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{

	std::string prop;
	switch (lPropNum)
	{
	case ePropVersion:
		setWStringToTVariant(pvarPropVal, m_version);
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
	case ePropReplyTo: {
		prop = client.getMsgProp(lPropNum);
		std::wstring wmsg = Utils::stringToWs(prop);
		setWStringToTVariant(pvarPropVal, wmsg.c_str());
		break;
	}
	default:
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------//
bool AddInNative::SetPropVal(const long lPropNum, tVariant *varPropVal)
{
	switch (lPropNum)
	{
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
	{
		std::string propVal = inputParamToStr(varPropVal, 0);
		client.setMsgProp(lPropNum, propVal);
		break;
	}
	default:
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------//
bool AddInNative::IsPropReadable(const long lPropNum)
{
	return true;
}

//---------------------------------------------------------------------------//
bool AddInNative::IsPropWritable(const long lPropNum)
{
	switch (lPropNum)
	{
	case ePropVersion:
		return false;
	case ePropCorrelationId:
		return true;
	default:
		return true;
	}
}

//---------------------------------------------------------------------------//
long AddInNative::GetNMethods()
{
	return eMethLast;
}

//---------------------------------------------------------------------------//
long AddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	long plMethodNum = -1;
	wchar_t* name = 0;
	convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

	delete[] name;

	return plMethodNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* AddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsMethodName = NULL;

	switch (lMethodAlias)
	{
	case 0: // First language (english)
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language (local)
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	uint32_t iActualSize = static_cast<uint32_t>(wcslen(wsCurrentName) + 1);

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
			convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	if (debugMode) {
		convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
		return wsMethodName;
	}

	return wsMethodName;
}

//---------------------------------------------------------------------------//
long AddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethGetLastError:
		return 0;
	case eMethConnect:
		return 7;
	case eMethDeclareQueue:
		return 7;
	case eMethBasicPublish:
		return 6;
	case eMethBasicConsume:
		return 5;
	case eMethBasicConsumeMessage:
		return 4;
	case eMethBasicCancel:
		return 1;
	case eMethBasicAck:
		return 1;
	case eMethBasicReject:
		return 1;
	case eMethDeleteQueue:
		return 3;
	case eMethBindQueue:
		return 4;
	case eMethDeclareExchange:
		return 6;
	case eMethDeleteExchange:
		return 2;
	case eMethUnbindQueue:
		return 3;
	case eMethSetPriority:
		return 1;
	default:
		return 0;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,	tVariant *pvarParamDefValue)
{
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;

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
		return false;
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
		return false;
	case eMethDeclareExchange:
		if (lParamNum == 5) {
			TV_VT(pvarParamDefValue) = VTYPE_PWSTR;
			TV_WSTR(pvarParamDefValue) = nullptr;
			pvarParamDefValue->wstrLen = 0;
			return true;
		}
		return false;
	case eMethBindQueue:
		if (lParamNum == 3) {
			TV_VT(pvarParamDefValue) = VTYPE_PWSTR;
			TV_WSTR(pvarParamDefValue) = nullptr;
			pvarParamDefValue->wstrLen = 0;
			return true;
		}
		return false;
	case eMethBasicPublish:
		if (lParamNum == 5) {
			TV_VT(pvarParamDefValue) = VTYPE_PWSTR;
			TV_WSTR(pvarParamDefValue) = nullptr;
			pvarParamDefValue->wstrLen = 0;
			return true;
		}
		return false;
	case eMethGetLastError:
		return false;
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::HasRetVal(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethGetLastError:
	case eMethBasicConsume:
	case eMethBasicConsumeMessage:
	case eMethDeclareQueue:
	case eMethGetPriority:
		return true;
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	if (!validateInputParameters(paParams, lMethodNum, lSizeArray))
		return false;

	switch (lMethodNum)
	{
		case eMethConnect:
		{
			std::string host = inputParamToStr(paParams, 0);
			uint16_t port = (uint16_t) paParams[1].intVal;
			std::string login = inputParamToStr(paParams, 2);
			std::string pwd = inputParamToStr(paParams, 3);
			std::string vhost = inputParamToStr(paParams, 4);
			bool ssl = paParams[6].bVal;
			return client.connect(host, port, login, pwd, vhost, ssl);
		}
		case eMethBasicPublish:
		{
			std::string exchange = inputParamToStr(paParams, 0);
			std::string routingKey = inputParamToStr(paParams, 1);
			std::string message = inputParamToStr(paParams, 2);
			std::string props = inputParamToStr(paParams, 5);
			return client.basicPublish(exchange, routingKey, message, paParams[4].bVal, props);
			return true;
		}
		case eMethBasicCancel:
		{
			return client.basicCancel();
		}
		case eMethBasicAck:
		{
			auto res = client.basicAck(paParams[0].ullVal);
			return res;
		}
		case eMethBasicReject:
		{
			return client.basicReject(paParams[0].ullVal);
		}
		case eMethDeleteQueue:
		{
			std::string name = inputParamToStr(paParams, 0);
			return client.deleteQueue(name,	paParams[1].bVal, paParams[2].bVal);
		}
		case eMethBindQueue: 
		{
			std::string queue = inputParamToStr(paParams, 0);
			std::string exchange = inputParamToStr(paParams, 1);
			std::string routingKey = inputParamToStr(paParams, 2);
			std::string props = inputParamToStr(paParams, 3);
			return client.bindQueue(queue, exchange, routingKey, props);
		}
		case eMethUnbindQueue:
		{
			std::string queue = inputParamToStr(paParams, 0);
			std::string exchange = inputParamToStr(paParams, 1);
			std::string routingKey = inputParamToStr(paParams, 2);
			return client.unbindQueue(queue, exchange, routingKey);
		}
		case eMethDeclareExchange:
		{
			std::string name = inputParamToStr(paParams, 0);
			std::string type = inputParamToStr(paParams, 1);
			std::string props = inputParamToStr(paParams, 5);
			return client.declareExchange(name, type, paParams[2].bVal, paParams[3].bVal, paParams[4].bVal, props);
		}
		case eMethDeleteExchange:
		{
			std::string name = inputParamToStr(paParams, 0);
			return client.deleteExchange(name, paParams[1].bVal);
		}
		case eMethSetPriority:
		{
			return client.setPriority(paParams[0].intVal);
		}
		default:
			return false;
	}
}

std::string AddInNative::inputParamToStr(tVariant* paParams, int parIndex) {
	WcharWrapper wWrapper(paParams[parIndex].pwstrVal);
	std::string res = Utils::wsToString(wWrapper);
	return res;
}

//---------------------------------------------------------------------------//
bool AddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	if (!validateInputParameters(paParams, lMethodNum, lSizeArray))
		return false;

	switch (lMethodNum)
	{
	case eMethGetLastError:
		return getLastError(pvarRetValue);
	case eMethBasicConsume:
		return basicConsume(pvarRetValue, paParams);
	case eMethBasicConsumeMessage:
		return basicConsumeMessage(pvarRetValue, paParams);
	case eMethDeclareQueue:
		return declareQueue(pvarRetValue, paParams);
	case eMethGetPriority:
		return getPriority(pvarRetValue, paParams);
	default:
		return false;
	}

	return false;
}

bool AddInNative::getLastError(tVariant* pvarRetValue) {
	std::string error = client.getLastError();
	setWStringToTVariant(pvarRetValue, Utils::stringToWs(error).c_str());
	TV_VT(pvarRetValue) = VTYPE_PWSTR;
	return true;
}

bool AddInNative::basicConsume(tVariant* pvarRetValue, tVariant* paParams) {
	
	std::string queue = inputParamToStr(paParams, 0);

	std::string channelId = client.basicConsume(queue,	paParams[4].intVal);

	if (client.getLastError().length() != 0) {
		return false;
	}

	setWStringToTVariant(pvarRetValue, Utils::stringToWs(channelId).c_str());
	TV_VT(pvarRetValue) = VTYPE_PWSTR;
	
	return true;
}

bool AddInNative::declareQueue(tVariant* pvarRetValue, tVariant* paParams) {
	std::string name = inputParamToStr(paParams, 0);
	std::string props = inputParamToStr(paParams, 6);
	std::string queueName = client.declareQueue(
		name,
		paParams[1].bVal,
		paParams[2].bVal,
		paParams[4].bVal,
		paParams[5].ushortVal,
		props
	);

	setWStringToTVariant(pvarRetValue, Utils::stringToWs(queueName).c_str());
	TV_VT(pvarRetValue) = VTYPE_PWSTR;
	
	if (client.getLastError().length() != 0) {
		return false;
	}
	
	return true;
}

bool AddInNative::basicConsumeMessage(tVariant* pvarRetValue, tVariant* paParams) {
	std::string outdata;
	std::uint64_t outMessageTag;
	
	bool hasMessage = client.basicConsumeMessage(
		outdata,
		outMessageTag,
		paParams[3].intVal
	);

	if (client.getLastError().length() != 0) {
		return false;
	}

	if (hasMessage)
	{
		setWStringToTVariant(&paParams[1], Utils::stringToWs(outdata).c_str());
	}
	TV_VT(&paParams[1]) = VTYPE_PWSTR; // Передаем сообщение в исходящий параметр 1С-ого метода

	TV_VT(&paParams[2]) = VTYPE_UI4;
	TV_UI8(&paParams[2]) = outMessageTag;

	TV_VT(pvarRetValue) = VTYPE_BOOL; // Устаналиваем тип возвращаемого значения 1С-ого метода в булево
	TV_BOOL(pvarRetValue) = hasMessage; // передаем возвращаемое значение 1с-ого метода как булево

	if (debugMode) {
		TV_VT(&paParams[1]) = VTYPE_PSTR;
		TV_STR(&paParams[1]) = Utils::stringToChar(outdata);
	}
	
	return true; // True узначает, что метод выполнился успешно
}

bool AddInNative::getPriority(tVariant* pvarRetValue, tVariant* paParams) {
	int priority = client.getPriority();

	TV_VT(pvarRetValue) = VTYPE_I4;
	TV_INT(pvarRetValue) = priority; 

	return true;
}

void AddInNative::setWStringToTVariant(tVariant* dest, const wchar_t* source) {

	size_t len = ::wcslen(source) + 1;

	TV_VT(dest) = VTYPE_PWSTR;

	if (m_iMemory) 
	{
		if (m_iMemory->AllocMemory((void**)&dest->pwstrVal, len * sizeof(WCHAR_T)))
			convToShortWchar(&dest->pwstrVal, source, len);
	}
	dest->wstrLen = ::wcslen(source);

}

//---------------------------------------------------------------------------//
bool AddInNative::validateInputParameters(tVariant* paParams, long const lMethodNum, long const lSizeArray) {

	switch (lMethodNum)
	{
	case eMethConnect:
		return validateConnect(paParams, lMethodNum, lSizeArray);
	case eMethBasicPublish:
		return validateBasicPublish(paParams, lMethodNum, lSizeArray);
	case eMethBasicCancel:
		return checkInputParameter(paParams, lMethodNum, 0, VTYPE_PWSTR);
	case eMethBasicAck:
		return true;
	case eMethBasicReject:
		return true;
	case eMethDeleteQueue:
	case eMethDeclareQueue:
		return validateDeclDelQueue(paParams, lMethodNum, lSizeArray);
	case eMethBindQueue:
	case eMethUnbindQueue:
		return validateBindUnbindQueue(paParams, lMethodNum, lSizeArray);
	case eMethDeclareExchange:
		return validateDeclareExchange(paParams, lMethodNum, lSizeArray);
	case eMethDeleteExchange:
		return validateDeleteExchange(paParams, lMethodNum, lSizeArray);
	case eMethBasicConsumeMessage:
		return validateBasicConsumeMessage(paParams, lMethodNum, lSizeArray);
	case eMethBasicConsume:
		return validateBasicConsume(paParams, lMethodNum, lSizeArray);
	default:
		return true;
	}
}

bool AddInNative::validateBasicConsume(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i == 2 || i == 3)
		{
			typeCheck = VTYPE_BOOL;
		}
		if (i == 4)
		{
			typeCheck = VTYPE_I4;
		}
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateBasicConsumeMessage(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i == 3)
		{
			typeCheck = VTYPE_I4;
		}
		if (i == 1 || i == 2)
		{
			// This is output parameter with type VTYPE_EMPTY. Skip it
			continue;
		}

		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateDeclDelQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_BOOL;
		if (i == 0 || i == 6)
		{
			typeCheck = VTYPE_PWSTR;
		}
		else if (i == 5)
		{
			typeCheck = VTYPE_I4;
		}
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateDeleteExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i == 1)
		{
			typeCheck = VTYPE_BOOL;
		}
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateDeclareExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i > 1 && i < 5)
		{
			typeCheck = VTYPE_BOOL;
		}
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateBindUnbindQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateConnect(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i == 1 || i == 5)
		{
			typeCheck = VTYPE_I4;
		}
		else if (i == 6) 
		{
			typeCheck = VTYPE_BOOL;
		}
		else 
		{
			typeCheck = VTYPE_PWSTR;
			if (paParams[i].intVal == 0) {
				return false;
			}
		}
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::validateBasicPublish(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i == 3) {
			typeCheck = VTYPE_I4;
		}
		else if (i == 4) {
			typeCheck = VTYPE_BOOL;
		}
		if (!checkInputParameter(paParams, lMethodNum, i, typeCheck))
		{
			return false;
		}
	}
	return true;
}

bool AddInNative::checkInputParameter(tVariant* params, long const methodNum, long const parameterNum, ENUMVAR type) {

	if (debugMode) {
		return true;
	}

	if (!(TV_VT(&params[parameterNum]) == type)) {
		const WCHAR_T* methName = GetMethodName(methodNum, 1);
		const wchar_t* methodName = WcharWrapper(methName);

		std::string errDescr = "Error occured when calling method "
			+ Utils::wsToString(methodName)
			+ "() - wrong type for parameter number "
			+ Utils::anyToString(parameterNum);
		
		if (methName && m_iMemory){
			m_iMemory->FreeMemory((void**)&methName);
		}

		addError(ADDIN_E_FAIL, L"NativeRabbitMQ", Utils::stringToWs(errDescr).c_str(), 1);
		client.updateLastError(errDescr.c_str());
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// ILocaleBase
//---------------------------------------------------------------------------//
void AddInNative::SetLocale(const WCHAR_T* loc)
{
}

/////////////////////////////////////////////////////////////////////////////
// Other

//---------------------------------------------------------------------------//
void AddInNative::addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T *err = 0;
		WCHAR_T *descr = 0;

		convToShortWchar(&err, source);
		convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);

		delete[] descr;
		delete[] err;
	}
}

//---------------------------------------------------------------------------//
long AddInNative::findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}

void AddInNative::ToV8String(const wchar_t* wstr, tVariant* par)
{
	if (wstr)
	{
		int len = wcslen(wstr);
		m_iMemory->AllocMemory((void**)&par->pwstrVal, (len + 1) * sizeof(WCHAR_T));
		convToShortWchar(&par->pwstrVal, wstr);
		par->vt = VTYPE_PWSTR;
		par->wstrLen = len;
	}
	else
		par->vt = VTYPE_EMPTY;
}

bool AddInNative::isNumericParameter(tVariant* par)
{
	return par->vt == VTYPE_I4 || par->vt == VTYPE_UI4 || par->vt == VTYPE_R8;
}

long AddInNative::numericValue(tVariant* par)
{
	long ret = 0;
	switch (par->vt)
	{
	case VTYPE_I4:
		ret = par->lVal;
		break;
	case VTYPE_UI4:
		ret = par->ulVal;
		break;
	case VTYPE_R8:
		ret = par->dblVal;
		break;
	}
	return ret;
}

void AddInNative::enableDebugMode() {
	debugMode = true;
}