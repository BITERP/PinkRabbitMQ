#ifndef __CCOMPONENTNATIVE_H__
#define __CCOMPONENTNATIVE_H__

#include "RabbitMQClient.h"
#include <addin/ComponentBase.h>
#include <addin/AddInDefBase.h>

typedef void* VOID_PTR;

///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class RabbitMQClientNative : public IComponentBase {
public:
	static const char16_t* componentName;

	enum Props {
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

	enum Methods {
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
		eMethGetHeaders,
		eMethLast      // Always last
	};

	RabbitMQClientNative(void);

	virtual ~RabbitMQClientNative();

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
		tVariant* pvarParamDefValue);

	virtual bool ADDIN_API HasRetVal(const long lMethodNum);

	virtual bool ADDIN_API CallAsProc(const long lMethodNum,
		tVariant* paParams, const long lSizeArray);

	virtual bool ADDIN_API CallAsFunc(const long lMethodNum,
		tVariant* pvarRetValue, tVariant* paParams,
		const long lSizeArray);

	// LocaleBase
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc);

private:

	RabbitMQClient impl;

	long findName(const char16_t* names[], u16string name, const uint32_t size) const;

};

#endif //__CCOMPONENTNATIVE_H__
