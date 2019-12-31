#include <string>
#include "src/AddInNative.h"
#include "src/ConversionWchar.h"
#include "src/Utils.h"
#include <fstream>

class RabbitMQClientTest
{
public:

	explicit RabbitMQClientTest()
	{
		testMessage;
		std::ifstream myReadFile;
		myReadFile.open("/home/rkudakov/projects/PinkRabbitMQLinux/src/testMessage560kb.txt");
		while (!myReadFile.eof()) {
			std::string msgLine;
			getline(myReadFile, msgLine);
			testMessage += msgLine;
		}
		myReadFile.close();
	}

	void testDeclareSendReceive() {

		native = new AddInNative();
		native->enableDebugMode();

		testConnect();
		testDeleteQueue();
		testDeleteExchange();
		testDeclareExchange();
		testDeclareQueue();
		testBindQueue();
		testSetProps(AddInNative::Props::ePropAppId);
		testSetProps(AddInNative::Props::ePropClusterId);
		testSetProps(AddInNative::Props::ePropContentEncoding);
		testSendReceiveSingle();

		delete native;
	}

	void testPassEmptyParameters() {

		native = new AddInNative();
		native->enableDebugMode();

		tVariant* params = new tVariant[6];
		params[0].pwstrVal = NULL;
		params[1].intVal = NULL;
		params[2].pwstrVal = NULL;
		params[3].pwstrVal = NULL;
		params[4].pwstrVal = NULL;
		params[5].intVal = NULL;

		bool result = native->CallAsProc(AddInNative::Methods::eMethConnect, params, 6);
		assertTrue(result == false, "testPassEmptyParameters");

		delete native;
	}

	void tesSendMessage() {

		native = new AddInNative();
		native->enableDebugMode();

		testConnect();
		testDeleteQueue();
		testDeleteExchange();
		testDeclareExchange();
		testDeclareQueue();
		testBindQueue();
		std::wstring wmsg = Utils::stringToWs(testMessage);

		wchar_t* messageToPublish = Utils::wstringToWchar(wmsg);

		for (int i = 1; i <= 5; i++) {
			testBasicPublish(messageToPublish, i);
		}

		delete native;
	}

private:

	void testSendReceiveSingle() {
		
		int out;
		std::wstring wmsg = Utils::stringToWs(testMessage);
		wchar_t* messageToPublish = Utils::wstringToWchar(wmsg);
		for (out = 1; out <= 5; out++) {
			testBasicPublish(messageToPublish, out);
		}

		testBasicConsume();

		std::string outdata;
		std::uint64_t outMessageTag;
		int in = 1;
		while (testBasicConsumeMessage(outdata, outMessageTag, in)) {
			assertTrue(outdata == testMessage, "Message text that were sent and read are equal");

			testBasicAck(outMessageTag);
			in++;

			outdata = "";
		}

		assertTrue(in == out, "Sent amount corresponding to received amount =" + Utils::anyToString(in));

		testBasicCancel();
	}

	void testConnect() {
		tVariant* params = new tVariant[6];

		WcharWrapper wHost(host);
		params[0].pwstrVal = wHost;
		params[1].uintVal = 5672;
		WcharWrapper wUsr(usr);
		params[2].pwstrVal = wUsr;
		WcharWrapper wPwd(pwd);
		params[3].pwstrVal = wPwd;
		WcharWrapper wVhost(vhost);
		params[4].pwstrVal = wVhost;

		bool result = native->CallAsProc(AddInNative::Methods::eMethConnect, params, 6);
		assertTrue(result == true, "testConnect");		
	}

	void testDeclareQueue() {
		tVariant* returnValue = new tVariant;
		tVariant* params = new tVariant[6];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		params[1].bVal = false;
		params[2].bVal = true;
		params[3].bVal = false;
		params[4].bVal = false;
		params[5].uintVal = 0;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethDeclareQueue, returnValue, params, 6);
		assertTrue(result == true, "testDeclareQueue");
	}

	void testDeleteQueue() {
		tVariant* params = new tVariant[3];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		params[1].bVal = false;
		params[2].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeleteQueue, params, 3);
		assertTrue(result == true, "testDeleteQueue");
	}

	void testDeclareExchange() {
		tVariant* params = new tVariant[5];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		WcharWrapper wExchangeType(L"topic");
		params[1].pwstrVal = wExchangeType;
		params[2].bVal = false;
		params[3].bVal = true;
		params[4].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeclareExchange, params, 5);


		assertTrue(result == true, "testDeclareExchange");
	}

	void testDeleteExchange() {
		tVariant* params = new tVariant[2];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		params[1].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeleteExchange, params, 2);
		assertTrue(result == true, "testDeleteExchange");
	}

	void testBindQueue() {
		tVariant* params = new tVariant[3];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		params[1].pwstrVal = wQueueExchange;
		WcharWrapper wRoutingKey(L"#");
		params[2].pwstrVal = wRoutingKey;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBindQueue, params, 3);
		assertTrue(result == true, "testBindQueue");
	}

	void testBasicPublish(wchar_t* message, int count) {
		tVariant* params = new tVariant[5];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		params[1].pwstrVal = wQueueExchange;
		WcharWrapper wMessage(message);
		params[2].pwstrVal = wMessage;
		params[3].uintVal = 0;
		params[4].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicPublish, params, 5);
	
		assertTrue(result == true, "testBasicPublish " + Utils::anyToString(count));
	}

	void testSetProps(int propNum) {

		tVariant* propVar = new tVariant();
		propVar->pstrVal = "test_prop";
		bool setResult = native->SetPropVal(propNum, propVar);

		tVariant propVarRet;
		bool getResult = native->GetPropVal(propNum, &propVarRet);

		assertTrue(getResult && setResult, "testSetProps");
	}

	void testBasicConsume() {
		
		tVariant* returnValue = new tVariant;
		tVariant* params = new tVariant[5];

		WcharWrapper wQueueExchange(queueExchange);
		params[0].pwstrVal = wQueueExchange;
		params[4].uintVal= 100;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethBasicConsume, returnValue, params, 5);

		assertTrue(result, "testBasicConsume");
	}

	uint64_t testBasicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, int i) {
		
		tVariant* returnValue = new tVariant;
		tVariant params[4] = {};

		params[3].uintVal = 3000;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethBasicConsumeMessage, returnValue, params, 4);

		outdata = params[1].pstrVal;
		outMessageTag = params[2].ullVal;

		if (params[1].pstrVal)
			delete[] params[1].pstrVal;

		assertTrue(result, "testBasicConsumeMessage " + Utils::anyToString(i));
		return returnValue->bVal;
	}

	void testBasicCancel() {
		tVariant* params = new tVariant[0];

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicCancel, params, 0);

		assertTrue(result == true, "testBasicCancel");
	}

	void testBasicAck(uint64_t messageTag) {
		tVariant* params = new tVariant[1];
		params[0].ullVal = messageTag;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicAck, params, 1);

		assertTrue(result == true, "testBasicAck");
	}

	void testBasicReject(uint64_t messageTag) {
		tVariant* params = new tVariant[1];
		params[0].ullVal = messageTag;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicReject, params, 1);

		assertTrue(result == true, "testBasicReject");
	}

private:
	wchar_t* host = L"devdevopsrmq.bit-erp.loc";
	int port = 5672;
	wchar_t* usr = L"rkudakov_devops";
	wchar_t* pwd = L"rkudakov_devops";
	wchar_t* vhost = L"rkudakov_devops";
	wchar_t* queueExchange = L"unit_test_linux";
	AddInNative* native;
	std::string testMessage;

	void assertTrue(bool condition, std::string msg) {
		assert(condition);
		
		auto now = std::chrono::system_clock::now();
		std::time_t end_time = std::chrono::system_clock::to_time_t(now);
		
		std::cout << std::ctime(&end_time) << " TEST PASSED: " << msg << std::endl;
	};
};
