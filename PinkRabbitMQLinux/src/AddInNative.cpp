

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
	L"DeviceInfo"
};

static const wchar_t* g_PropNamesRu[] =
{
	L"ОписаниеУстройства"
};

static const wchar_t *g_MethodNames[] =
{
	L"Connect",
};


static const wchar_t *g_MethodNamesRu[] =
{
	L"Connect",
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
	switch (lPropNum)
	{
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::SetPropVal(const long lPropNum, tVariant *varPropVal)
{
	switch (lPropNum)
	{
	default:
		return false;
	}
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
	default:
		return false;
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

	return wsMethodName;
}

//---------------------------------------------------------------------------//
long AddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
		case eMethConnect:
			return 6;
		default:
			return 0;
		}
}

//---------------------------------------------------------------------------//
bool AddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,	tVariant *pvarParamDefValue)
{
	switch (lMethodNum)
	{
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::HasRetVal(const long lMethodNum)
{
	switch (lMethodNum)
	{
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
		case eMethConnect:
		{
			std::string host = inputParamToStr(paParams, 0);
			std::string login = inputParamToStr(paParams, 2);
			std::string pwd = inputParamToStr(paParams, 3);
			std::string vhost = inputParamToStr(paParams, 4);
			return client.connect(
				host,
				paParams[1].uintVal,
				login,
				pwd,
				vhost
			);
			return true;
		}
		default:
			return false;
	}
}

std::string AddInNative::inputParamToStr(tVariant* paParams, int parIndex) {
	if (TV_VT(&paParams[parIndex]) == VTYPE_PWSTR) {
		wchar_t* parVal = WcharWrapper(paParams[parIndex].pwstrVal);
		return Utils::wsToString(parVal);
	}
	else {
		return paParams[parIndex].pstrVal;
	}
}

//---------------------------------------------------------------------------//
bool AddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
		default:
			return false;
	}

	return false;
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