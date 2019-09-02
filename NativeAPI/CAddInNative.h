#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "Utils.h"
#include "IMemoryManager.h"
#include "RabbitMQClient.h"

typedef void* VOID_PTR;

///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class CAddInNative : public IComponentBase
{
public:
    enum Props
    {
        ePropVersion = 0,
		ePropCorrelationId = 1,
		ePropType = 2,
		ePropMessageId = 3,
		ePropAppId = 4,
		ePropContentEncoding = 5,
		ePropContentType = 6,
		ePropUserId = 7,
		ePropClusterId = 8,
		ePropExpiration = 9,
		ePropReplyTo = 10,
        ePropLast = 11      // Always last
    };

    enum Methods
    {
        eMethGetLastError = 0,
		eMethConnect = 1,
		eMethDeclareQueue = 2,
		eMethBasicPublish = 3,
		eMethBasicConsume = 4,
		eMethBasicConsumeMessage = 5,
		eMethBasicCancel = 6,
		eMethBasicAck = 7,
		eMethDeleteQueue = 8,
		eMethBindQueue = 9,
		eMethBasicReject = 10,
		eMethDeclareExchange = 11,
		eMethDeleteExchange = 12,
		eMethUnbindQueue = 13,
        eMethLast = 14      // Always last
    };

    CAddInNative(void);
    virtual ~CAddInNative();
    // IInitDoneBase
    virtual bool ADDIN_API Init(VOID_PTR);
    virtual bool ADDIN_API setMemManager(void* mem);
    virtual long ADDIN_API GetInfo();
    virtual void ADDIN_API Done();
    // ILanguageExtenderBase
    virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T**);
    virtual long ADDIN_API GetNProps();
    virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
    virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
    virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal);
    virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal);
    virtual bool ADDIN_API IsPropReadable(const long lPropNum);
    virtual bool ADDIN_API IsPropWritable(const long lPropNum);
    virtual long ADDIN_API GetNMethods();
    virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
    virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, 
                            const long lMethodAlias);
    virtual long ADDIN_API GetNParams(const long lMethodNum);
    virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum,
                            tVariant *pvarParamDefValue);   
    virtual bool ADDIN_API HasRetVal(const long lMethodNum);
    virtual bool ADDIN_API CallAsProc(const long lMethodNum,
                    tVariant* paParams, const long lSizeArray);
    virtual bool ADDIN_API CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
    // LocaleBase
    virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
	void CAddInNative::setWStringToTVariant(tVariant* dest, const wchar_t* source);
    
private:

	RabbitMQClient* client;

    long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
	bool checkInputParameter(tVariant* params, const long methodNum, const long index, ENUMVAR type);
    void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);
	bool validateInputParameters(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool getLastError(tVariant* pvarRetValue);
	bool basicConsume(tVariant* pvarRetValue, tVariant* paParams);
	bool basicConsumeMessage(tVariant* pvarRetValue, tVariant* paParams);
	bool declareQueue(tVariant* pvarRetValue, tVariant* paParams);
	bool validateConnect(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBasicPublish(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateDeclDelQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBindUnbindQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateDeclareExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateDeleteExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBasicConsumeMessage(tVariant* paParams, long const lMethodNum, long const lSizeArray);
	bool validateBasicConsume(tVariant* paParams, long const lMethodNum, long const lSizeArray);

    // Attributes
    IAddInDefBase      *m_iConnect;
    IMemoryManager     *m_iMemory;

	const wchar_t*      m_version = L"1.4";
};

class WcharWrapper
{
public:
#ifdef LINUX_OR_MACOS
    WcharWrapper(const WCHAR_T* str);
#endif
    explicit WcharWrapper(const wchar_t* str);
	WcharWrapper(const WcharWrapper &) = delete;
	WcharWrapper(const WcharWrapper &&) = delete;
    ~WcharWrapper();
#ifdef LINUX_OR_MACOS
    operator const WCHAR_T*(){ return m_str_WCHAR; }
    operator WCHAR_T*(){ return m_str_WCHAR; }
#endif
    explicit operator const wchar_t*(){ return m_str_wchar; }
	explicit operator wchar_t*(){ return m_str_wchar; }
	WcharWrapper &operator==(const WcharWrapper &) = delete;
	WcharWrapper &operator==(const WcharWrapper &&) = delete;

#ifdef LINUX_OR_MACOS
    WCHAR_T* m_str_WCHAR;
#endif
    wchar_t* m_str_wchar;
};
#endif //__ADDINNATIVE_H__
