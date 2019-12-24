#include <string>
#include "src/AddInNative.h"
#include "src/ConversionWchar.h"
#include "src/Utils.h"

class RabbitMQClientTest
{
public:

	explicit RabbitMQClientTest()
	{

	}

	void testSendReceive() {
		testConnect();
		testDeleteQueue();
		testDeleteExchange();
		testDeclareExchange();
		testDeclareQueue();
		testBindQueue();
	//	testSetProps(AddInNative::Props::ePropAppId);
	//	testSetProps(AddInNative::Props::ePropClusterId);
//		testSetProps(AddInNative::Props::ePropContentEncoding);
		testBasicPublish();

	}

	void testConnect() {
		tVariant* params = new tVariant[6];

		params[0].pstrVal = host;
		params[1].uintVal = 5672;
		params[2].pstrVal = usr;
		params[3].pstrVal = pwd;
		params[4].pstrVal = vhost;

		bool result = native.CallAsProc(AddInNative::Methods::eMethConnect, params, sizeof(params));
		assertTrue(result == true, "testConnect");		
	}

	void testDeclareQueue() {
		tVariant* returnValue = new tVariant;
		tVariant* params = new tVariant[6];

		params[0].pstrVal = queueExchange;
		params[1].bVal = false;
		params[2].bVal = false;
		params[3].bVal = false;
		params[4].bVal = false;
		params[5].uintVal = 0;

		bool result = native.CallAsFunc(AddInNative::Methods::eMethDeclareQueue, returnValue, params, sizeof(params));
		assertTrue(result == true, "testDeclareQueue");
	}

	void testDeleteQueue() {
		tVariant* params = new tVariant[3];

		params[0].pstrVal = queueExchange;
		params[1].bVal = false;
		params[2].bVal = false;

		bool result = native.CallAsProc(AddInNative::Methods::eMethDeleteQueue, params, sizeof(params));
		assertTrue(result == true, "testDeleteQueue");
	}

	void testDeclareExchange() {
		tVariant* params = new tVariant[5];

		params[0].pstrVal = queueExchange;
		params[1].pstrVal = "topic";
		params[2].bVal = false;
		params[3].bVal = false;
		params[4].bVal = false;

		bool result = native.CallAsProc(AddInNative::Methods::eMethDeclareExchange, params, sizeof(params));
		assertTrue(result == true, "testDeclareExchange");
	}

	void testDeleteExchange() {
		tVariant* params = new tVariant[2];

		params[0].pstrVal = queueExchange;
		params[1].bVal = false;

		bool result = native.CallAsProc(AddInNative::Methods::eMethDeleteExchange, params, sizeof(params));
		assertTrue(result == true, "testDeleteExchange");
	}

	void testBindQueue() {
		tVariant* params = new tVariant[3];

		params[0].pstrVal = queueExchange;
		params[1].pstrVal = queueExchange;
		params[2].pstrVal = "#";

		bool result = native.CallAsProc(AddInNative::Methods::eMethBindQueue, params, sizeof(params));
		assertTrue(result == true, "testBindQueue");
	}

	void testBasicPublish() {
		tVariant* params = new tVariant[5];

		params[0].pstrVal = queueExchange;
		params[1].pstrVal = queueExchange;
		params[2].pstrVal = queueExchange;
		params[3].uintVal = 0;
		params[4].bVal = false;

		bool result = native.CallAsProc(AddInNative::Methods::eMethBasicPublish, params, sizeof(params));
	
		assertTrue(result == true, "testBasicPublish");
	}

	void testSetProps(int propNum) {

		tVariant* provVar = new tVariant();
		provVar->pstrVal = "test_prop";
		bool setResult = native.SetPropVal(propNum, provVar);

		tVariant* provVarRet;
		bool getResult = native.GetPropVal(propNum, provVarRet);

		assertTrue(getResult && setResult, "testSetProps");
	}

private:
	char* host = "devdevopsrmq.bit-erp.loc";
	int port = 5672;
	char* usr = "rkudakov_devops";
	char* pwd = "rkudakov_devops";
	char* vhost = "rkudakov_devops";
	char* queueExchange = "unit_test_linux";
	AddInNative native;

	void assertTrue(bool condition, std::string methodName) {
		assert(condition);
		std::cout << "TEST PASSED " << methodName << std::endl;
	};
};
