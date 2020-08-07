
#include "addin/stdafx.h"
#pragma warning( disable : 4267)
#pragma warning( disable : 4311)
#pragma warning( disable : 4302)


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
#include "CAddInNative.h"
#include <string>
#include "RabbitMQClient.h"
#include "Utils.h"
#include <codecvt>

constexpr size_t TIME_LEN = 65;

#define BASE_ERRNO    

static const wchar_t *g_PropNames[] = {
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
static const wchar_t *g_MethodNames[] = {
    L"GetLastError",
	L"Connect,",
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

static const wchar_t *g_PropNamesRu[] = {
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
static const wchar_t *g_MethodNamesRu[] = {
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

static const wchar_t g_kClassNames[] = L"PinkRabbitMQ";
static IAddInDefBase *pAsyncEvent = NULL;
static void *consumedMessage = NULL;

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);
//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* /*wsName*/, IComponentBase** pInterface)
{
    if(!*pInterface)
    {
        *pInterface= new CAddInNative;
        intptr_t res = reinterpret_cast<intptr_t>(*pInterface);
		return static_cast<long>(res);
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
long DestroyObject(IComponentBase** pIntf)
{
    if(!*pIntf)
        return -1;

    delete *pIntf;
    *pIntf = 0;
    return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
    return (const WCHAR_T*)s_names;
}
//---------------------------------------------------------------------------//
#if !defined( __linux__ ) && !defined(__APPLE__)
VOID CALLBACK MyTimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired);
#else
static void MyTimerProc(int sig);
#endif //__linux__

// CAddInNative
//---------------------------------------------------------------------------//
CAddInNative::CAddInNative()
{
    m_iMemory = 0;
    m_iConnect = 0;
}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(VOID_PTR pConnection)
{ 
	client = new RabbitMQClient();

    m_iConnect = (IAddInDefBase*)pConnection;
    return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{ 
    // Component should put supported component technology version 
    // This component supports 2.0 version
    return 2000; 
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{
	delete client;
}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{ 
    const wchar_t *wsExtension = g_kClassNames;
    size_t iActualSize = ::wcslen(wsExtension) + 1;

    if (m_iMemory)
    {
        if(m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
            ::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
        return true;
    }

    return false; 
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{ 
    // You may delete next lines and add your own implementation code here
    return ePropLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{ 
    long plPropNum = -1;
    wchar_t* propName = 0;

    ::convFromShortWchar(&propName, wsPropName);
    plPropNum = findName(g_PropNames, propName, ePropLast);

    if (plPropNum == -1)
        plPropNum = findName(g_PropNamesRu, propName, ePropLast);

    delete[] propName;

    return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{ 
    if (lPropNum >= ePropLast)
        return NULL;

    const wchar_t *wsCurrentName = NULL;
    WCHAR_T *wsPropName = NULL;
    int iActualSize = 0;

    switch(lPropAlias)
    {
    case 0: // First language
        wsCurrentName = g_PropNames[lPropNum];
        break;
    case 1: // Second language
        wsCurrentName = g_PropNamesRu[lPropNum];
        break;
    default:
        return 0;
    }
    
    iActualSize = wcslen(wsCurrentName) + 1;

    if (m_iMemory && wsCurrentName && (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))) {
		::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
    }

    return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{ 
	std::string prop;
    switch(lPropNum)
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
	case ePropReplyTo:
		prop = client->getMsgProp(lPropNum);
		setWStringToTVariant(pvarPropVal, Utils::stringToWs(prop).c_str());
		break;
    default:
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant *varPropVal)
{ 
    switch(lPropNum)
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
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		client->setMsgProp(lPropNum, Utils::wsToString(TV_WSTR(varPropVal)));
		break;
    default:
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long /*lPropNum*/)
{ 
    return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
    switch(lPropNum)
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
long CAddInNative::GetNMethods()
{ 
    return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{ 
    long plMethodNum = -1;
    wchar_t* name = 0;

    ::convFromShortWchar(&name, wsMethodName);

    plMethodNum = findName(g_MethodNames, name, eMethLast);

    if (plMethodNum == -1)
        plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

    delete[] name;

    return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{ 
    if (lMethodNum >= eMethLast)
        return NULL;

    const wchar_t *wsCurrentName = NULL;
    WCHAR_T *wsMethodName = NULL;
    int iActualSize = 0;

    switch(lMethodAlias)
    {
    case 0: // First language
        wsCurrentName = g_MethodNames[lMethodNum];
        break;
    case 1: // Second language
        wsCurrentName = g_MethodNamesRu[lMethodNum];
        break;
    default: 
        return 0;
    }

    iActualSize = wcslen(wsCurrentName) + 1;

    if (m_iMemory && wsCurrentName && (m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))) {
            ::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
    }

    return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{ 
    switch(lMethodNum)
    { 
	case eMethGetLastError:
		return 0;
	case eMethConnect:
		return 6;
	case eMethDeclareQueue:
		return 6;
	case eMethBasicPublish:
		return 5;
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
		return 3;
	case eMethDeclareExchange:
		return 5;
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
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
                        tVariant *pvarParamDefValue)
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
		return false;
	case eMethDeclareQueue:
		if (lParamNum == 5) {
			TV_VT(pvarParamDefValue) = VTYPE_I4;
			TV_I4(pvarParamDefValue) = 0;
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
bool CAddInNative::HasRetVal(const long lMethodNum)
{ 
    switch(lMethodNum)
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
bool CAddInNative::CallAsProc(const long lMethodNum,
	tVariant* paParams, const long lSizeArray) 
{

	if (!validateInputParameters(paParams, lMethodNum, lSizeArray))
		return false;

    switch(lMethodNum)
    { 
	case eMethConnect:
		return client->connect(
			Utils::wsToString(paParams[0].pwstrVal),
			paParams[1].uintVal, 
			Utils::wsToString(paParams[2].pwstrVal),
			Utils::wsToString(paParams[3].pwstrVal),
			Utils::wsToString(paParams[4].pwstrVal)
		);
	case eMethBasicPublish:
		return client->basicPublish(
			Utils::wsToString(paParams[0].pwstrVal),
			Utils::wsToString(paParams[1].pwstrVal),
			Utils::wsToString(paParams[2].pwstrVal),
			paParams[4].bVal
		);
	case eMethBasicCancel:
		return client->basicCancel();
	case eMethBasicAck:
	{
		auto res = client->basicAck(paParams[0].ullVal);
		return res;
	}
	case eMethBasicReject:
		return client->basicReject(paParams[0].ullVal);
	case eMethDeleteQueue:
		return client->deleteQueue(
			Utils::wsToString(paParams[0].pwstrVal),
			paParams[1].bVal,
			paParams[2].bVal
		);
	case eMethBindQueue:
		return client->bindQueue(
			Utils::wsToString(paParams[0].pwstrVal),
			Utils::wsToString(paParams[1].pwstrVal),
			Utils::wsToString(paParams[2].pwstrVal)
		);
	case eMethUnbindQueue:
		return client->unbindQueue(
			Utils::wsToString(paParams[0].pwstrVal),
			Utils::wsToString(paParams[1].pwstrVal),
			Utils::wsToString(paParams[2].pwstrVal)
		);
	case eMethDeclareExchange:
		return client->declareExchange(
			Utils::wsToString(paParams[0].pwstrVal),
			Utils::wsToString(paParams[1].pwstrVal),
			paParams[2].bVal,
			paParams[3].bVal,
			paParams[4].bVal
		);
	case eMethDeleteExchange:
		return client->deleteExchange(
			Utils::wsToString(paParams[0].pwstrVal),
			paParams[1].bVal
		);
	case eMethSetPriority:
		return client->setPriority(paParams[0].intVal);
    default:
        return false;
    }
}

//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{ 
	if (!validateInputParameters(paParams, lMethodNum, lSizeArray))
		return false;

    switch(lMethodNum)
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
}

bool CAddInNative::getLastError(tVariant* pvarRetValue) {
	wchar_t* error = client->getLastError();
	setWStringToTVariant(pvarRetValue, error);
	TV_VT(pvarRetValue) = VTYPE_PWSTR;
	return true;
}

bool CAddInNative::basicConsume(tVariant* pvarRetValue, tVariant* paParams) {
	std::string channelId = client->basicConsume(
		Utils::wsToString(paParams[0].pwstrVal),
		paParams[4].intVal
	);

	if (wcslen(client->getLastError()) != 0) {
		return false;
	}

	setWStringToTVariant(pvarRetValue, Utils::stringToWs(channelId).c_str());
	TV_VT(pvarRetValue) = VTYPE_PWSTR;
	return true;
}

bool CAddInNative::declareQueue(tVariant* pvarRetValue, tVariant* paParams) {
	std::string queueName = client->declareQueue(
		Utils::wsToString(paParams[0].pwstrVal),
		paParams[1].bVal,
		paParams[2].bVal,
		paParams[4].bVal,
		paParams[5].ushortVal
	);
	setWStringToTVariant(pvarRetValue, Utils::stringToWs(queueName).c_str());
	TV_VT(pvarRetValue) = VTYPE_PWSTR;

	if (wcslen(client->getLastError()) != 0) {
		return false;
	}
	return true;
}

bool CAddInNative::basicConsumeMessage(tVariant* pvarRetValue, tVariant* paParams) {
	std::string outdata;
	std::uint64_t outMessageTag;

	bool hasMessage = client->basicConsumeMessage(
		outdata,
		outMessageTag,
		paParams[3].intVal
	);

	if (wcslen(client->getLastError()) != 0) {
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
	return true; // True узначает, что метод выполнился успешно
}

bool CAddInNative::getPriority(tVariant* pvarRetValue, tVariant* paParams) {
	int priority = client->getPriority();

	return true;
}

bool CAddInNative::validateInputParameters(tVariant* paParams, long const lMethodNum, long const lSizeArray) {

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

bool CAddInNative::validateBasicConsume(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
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

bool CAddInNative::validateBasicConsumeMessage(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
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

bool CAddInNative::validateDeclDelQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_BOOL;
		if (i == 0)
		{
			typeCheck = VTYPE_PWSTR;
		} else if(i == 5)
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

bool CAddInNative::validateDeleteExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
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

bool CAddInNative::validateDeclareExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i > 1)
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

bool CAddInNative::validateBindUnbindQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
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

bool CAddInNative::validateConnect(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
	for (int i = 0; i < lSizeArray; i++)
	{
		ENUMVAR typeCheck = VTYPE_PWSTR;
		if (i == 1 || i == 5)
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

bool CAddInNative::validateBasicPublish(tVariant* paParams, long const lMethodNum, long const lSizeArray) {
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

bool CAddInNative::checkInputParameter(tVariant* params, long const methodNum, long const parameterNum, ENUMVAR type) {
	
	bool test = params[parameterNum].intVal == 0;

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

//---------------------------------------------------------------------------//
// This code will work only on the client!
#if !defined( __linux__ ) && !defined(__APPLE__)
VOID CALLBACK MyTimerProc(PVOID /*lpParam*/, BOOLEAN /*TimerOrWaitFired*/)
{
    if (!pAsyncEvent)
        return;

	wchar_t* who = L"ComponentNative";
	wchar_t* what = L"Timer";

    wchar_t *wstime = new wchar_t[TIME_LEN];
    if (wstime)
    {
        wmemset(wstime, 0, TIME_LEN);
        time_t vtime;
        time(&vtime);
        ::_ui64tow_s(vtime, wstime, TIME_LEN, 10);
        pAsyncEvent->ExternalEvent(who, what, wstime);
        delete[] wstime;
    }
}
#else
void MyTimerProc(int sig)
{
    if (!pAsyncEvent)
        return;

    WCHAR_T *who = 0, *what = 0, *wdata = 0;
    wchar_t *data = 0;
    time_t dwTime = time(NULL);

    data = new wchar_t[TIME_LEN];
    
    if (data)
    {
        wmemset(data, 0, TIME_LEN);
        swprintf(data, TIME_LEN, L"%ul", dwTime);
        ::convToShortWchar(&who, L"ComponentNative");
        ::convToShortWchar(&what, L"Timer");
        ::convToShortWchar(&wdata, data);

        pAsyncEvent->ExternalEvent(who, what, wdata);

        delete[] who;
        delete[] what;
        delete[] wdata;
        delete[] data;
    }
}
#endif
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
#if !defined( __linux__ ) && !defined(__APPLE__)
    _wsetlocale(LC_ALL, loc);
#else
    //We convert in char* char_locale
    //also we establish locale
    //setlocale(LC_ALL, char_locale);
#endif
}

void CAddInNative::setStringToTVariant(tVariant* dest, std::string source) {
	size_t len = source.length();
	TV_VT(dest) = VTYPE_PSTR;
	if (m_iMemory->AllocMemory((void**)& dest->pstrVal, len))
		memcpy((void*)dest->pstrVal, (void*)source.c_str(), len);

	dest->strLen = len;
}

void CAddInNative::setWStringToTVariant(tVariant* dest, const wchar_t* source) {

	size_t len = ::wcslen(source) + 1;

	TV_VT(dest) = VTYPE_PWSTR;

	if (m_iMemory->AllocMemory((void**)& dest->pwstrVal, len * sizeof(WCHAR_T)))
		convToShortWchar(&dest->pwstrVal, source, len);
	dest->wstrLen = ::wcslen(source);
}
/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
    m_iMemory = (IMemoryManager*)mem;
    return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source, 
                        const wchar_t* descriptor, long code)
{
    if (m_iConnect)
    {
        WCHAR_T *err = 0;
        WCHAR_T *descr = 0;
        
        ::convToShortWchar(&err, source);
        ::convToShortWchar(&descr, descriptor);

        m_iConnect->AddError(wcode, err, descr, code);
        delete[] err;
        delete[] descr;
    }
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name, 
                        const uint32_t size) const
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
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
    if (!len)
        len = ::wcslen(Source) + 1;

    if (!*Dest)
        *Dest = new WCHAR_T[len];

    WCHAR_T* tmpShort = *Dest;
    const wchar_t* tmpWChar = Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len * sizeof(WCHAR_T));
#ifdef __linux__
    size_t succeed = (size_t)-1;
    size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
    const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
    iconv_t cd = iconv_open("UTF-16LE", fromCode);
    if (cd != (iconv_t)-1)
    {
        succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return (uint32_t)succeed;
    }
#endif //__linux__
    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpShort = (WCHAR_T)*tmpWChar;
    }

    return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
    if (!len)
        len = getLenShortWcharStr(Source) + 1;

    if (!*Dest)
        *Dest = new wchar_t[len];

    wchar_t* tmpWChar = *Dest;
	const  WCHAR_T* tmpShort = Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len * sizeof(wchar_t));
#ifdef __linux__
    size_t succeed = (size_t)-1;
    const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
    size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
    iconv_t cd = iconv_open("UTF-32LE", fromCode);
    if (cd != (iconv_t)-1)
    {
        succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return (uint32_t)succeed;
    }
#endif //__linux__
    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpWChar = (wchar_t)*tmpShort;
    }

    return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
    uint32_t res = 0;
    const WCHAR_T *tmpShort = Source;

    while (*tmpShort++)
        ++res;

    return res;
}
//---------------------------------------------------------------------------//

#ifdef LINUX_OR_MACOS
WcharWrapper::WcharWrapper(const WCHAR_T* str) : m_str_WCHAR(NULL),
                           m_str_wchar(NULL)
{
    if (str)
    {
        int len = getLenShortWcharStr(str);
        m_str_WCHAR = new WCHAR_T[len + 1];
        memset(m_str_WCHAR,   0, sizeof(WCHAR_T) * (len + 1));
        memcpy(m_str_WCHAR, str, sizeof(WCHAR_T) * len);
        ::convFromShortWchar(&m_str_wchar, m_str_WCHAR);
    }
}
#endif
//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) :
#ifdef LINUX_OR_MACOS
    m_str_WCHAR(NULL),
#endif 
    m_str_wchar(NULL)
{
    if (str)
    {
        int len = wcslen(str);
	
        m_str_wchar = new wchar_t[len + 1];
        memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
        memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
#ifdef LINUX_OR_MACOS
        ::convToShortWchar(&m_str_WCHAR, m_str_wchar);
#endif
    }

}
//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
#ifdef LINUX_OR_MACOS
    if (m_str_WCHAR)
    {
        delete [] m_str_WCHAR;
        m_str_WCHAR = NULL;
    }
#endif
    if (m_str_wchar)
    {
        delete [] m_str_wchar;
        m_str_wchar = NULL;
    }
}
//---------------------------------------------------------------------------//
