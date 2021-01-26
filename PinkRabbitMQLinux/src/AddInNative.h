
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

#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include <string>
#include "../include/ComponentBase.h"
#include "../include/AddInDefBase.h"
#include "../include/IMemoryManager.h"
#include "RabbitMQClient.h"

class AddInNative : public IComponentBase
{
public:
    enum Props
    {
        ePropVersion = 0,
        ePropCorrelationId,
        ePropType,
        ePropMessageId,
        ePropAppId,
        ePropContentEncoding,
        ePropContentType,
        ePropUserId,
        ePropClusterId,
        ePropExpiration,
        ePropReplyTo,
        ePropLast // Always last
    };

    enum Methods
    {
        eMethGetLastError = 0,
        eMethConnect,
        eMethDeclareQueue,
        eMethBasicPublish,
        eMethBasicConsume,
        eMethBasicConsumeMessage,
        eMethBasicCancel,
        eMethBasicAck,
        eMethDeleteQueue,
        eMethBindQueue,
        eMethBasicReject,
        eMethDeclareExchange,
        eMethDeleteExchange,
        eMethUnbindQueue,
        eMethSetPriority,
        eMethGetPriority,
        eMethGetRoutingKey,
        eMethLast      // Always last
    };

    AddInNative(void);
    virtual ~AddInNative();
    // IInitDoneBase
    virtual bool ADDIN_API Init(void*);
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
    // ILocaleBase
    virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
    void enableDebugMode();

private:
    long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
    void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);

    bool isNumericParameter(tVariant*);
    long numericValue(tVariant*);

    void ToV8String(const wchar_t* wstr, tVariant*);

    IAddInDefBaseEx    *m_iConnect;
    IMemoryManager     *m_iMemory;

    RabbitMQClient client;
    const wchar_t* m_version = L"1.10";
    bool debugMode = false;

    void setWStringToTVariant(tVariant* dest, const wchar_t* source);
    std::string inputParamToStr(tVariant* paParams, int parIndex);

    bool getLastError(tVariant* pvarRetValue);
    bool basicConsume(tVariant* pvarRetValue, tVariant* paParams);
    bool basicConsumeMessage(tVariant* pvarRetValue, tVariant* paParams);
    bool getPriority(tVariant* pvarRetValue, tVariant* paParams);
    bool getRoutingKey(tVariant* pvarRetValue, tVariant* paParams);
    bool declareQueue(tVariant* pvarRetValue, tVariant* paParams);
    bool validateConnect(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateBasicPublish(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateDeclDelQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateBindUnbindQueue(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateDeclareExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateDeleteExchange(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateBasicConsumeMessage(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool validateBasicConsume(tVariant* paParams, long const lMethodNum, long const lSizeArray);
    bool checkInputParameter(tVariant* params, const long methodNum, const long index, ENUMVAR type);
    bool validateInputParameters(tVariant* paParams, long const lMethodNum, long const lSizeArray);

};

#endif
