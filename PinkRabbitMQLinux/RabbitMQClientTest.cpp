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
		testBasicPublish(Utils::wstringToWchar(wmsg), 1);

		delete native;
	}

	void testSendReceiveSingle() {
		


		int out;
		for (out = 1; out <= 100; out++) {
			std::wstring wmsg = Utils::stringToWs(testMessage);
			testBasicPublish(Utils::wstringToWchar(wmsg), out);
		}
		testBasicConsume();

		std::string outdata;
		std::uint64_t outMessageTag;
		int in = 1;
		while (testBasicConsumeMessage(outdata, outMessageTag, in)) {
			assertTrue(outdata == testMessage, "Message text that were sent and read are equal");

			testBasicAck(outMessageTag);
			in++;
		}

		assertTrue(in == out, "Sent amount corresponding to received amount =" + Utils::anyToString(in));

		testBasicCancel();
	}

	void testConnect() {
		tVariant* params = new tVariant[6];

		params[0].pwstrVal = WcharWrapper(host);
		params[1].uintVal = 5672;
		params[2].pwstrVal = WcharWrapper(usr);
		params[3].pwstrVal = WcharWrapper(pwd);
		params[4].pwstrVal = WcharWrapper(vhost);

		bool result = native->CallAsProc(AddInNative::Methods::eMethConnect, params, sizeof(params));
		assertTrue(result == true, "testConnect");		
	}

	void testDeclareQueue() {
		tVariant* returnValue = new tVariant;
		tVariant* params = new tVariant[6];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[1].bVal = false;
		params[2].bVal = false;
		params[3].bVal = false;
		params[4].bVal = false;
		params[5].uintVal = 0;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethDeclareQueue, returnValue, params, sizeof(params));
		assertTrue(result == true, "testDeclareQueue");
	}

	void testDeleteQueue() {
		tVariant* params = new tVariant[3];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[1].bVal = false;
		params[2].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeleteQueue, params, sizeof(params));
		assertTrue(result == true, "testDeleteQueue");
	}

	void testDeclareExchange() {
		tVariant* params = new tVariant[5];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[1].pwstrVal = WcharWrapper(L"topic");
		params[2].bVal = false;
		params[3].bVal = false;
		params[4].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeclareExchange, params, sizeof(params));
		assertTrue(result == true, "testDeclareExchange");
	}

	void testDeleteExchange() {
		tVariant* params = new tVariant[2];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[1].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeleteExchange, params, sizeof(params));
		assertTrue(result == true, "testDeleteExchange");
	}

	void testBindQueue() {
		tVariant* params = new tVariant[3];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[1].pwstrVal = WcharWrapper(queueExchange);
		params[2].pwstrVal = WcharWrapper(L"#");

		bool result = native->CallAsProc(AddInNative::Methods::eMethBindQueue, params, sizeof(params));
		assertTrue(result == true, "testBindQueue");
	}

	void testBasicPublish(wchar_t* message, int count) {
		tVariant* params = new tVariant[5];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[1].pwstrVal = WcharWrapper(queueExchange);
		params[2].pwstrVal = WcharWrapper(message);
		params[3].uintVal = 0;
		params[4].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicPublish, params, sizeof(params));
	
		assertTrue(result == true, "testBasicPublish " + Utils::anyToString(count));
	}

	void testSetProps(int propNum) {

		tVariant* propVar = new tVariant();
		propVar->pstrVal = "test_prop";
		bool setResult = native->SetPropVal(propNum, propVar);

		tVariant* propVarRet;
		bool getResult = native->GetPropVal(propNum, propVarRet);

		assertTrue(getResult && setResult, "testSetProps");
	}

	void testBasicConsume() {
		
		tVariant* returnValue = new tVariant;
		tVariant* params = new tVariant[5];

		params[0].pwstrVal = WcharWrapper(queueExchange);
		params[4].uintVal= 100;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethBasicConsume, returnValue, params, sizeof(params));

		assertTrue(result, "testBasicConsume");
	}

	uint64_t testBasicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, int i) {
		

		tVariant* returnValue = new tVariant;
		tVariant* params = new tVariant[4];

		params[3].uintVal = 3000;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethBasicConsumeMessage, returnValue, params, sizeof(params));

		outdata = params[1].pstrVal;
		outMessageTag = params[2].ullVal;

		assertTrue(result, "testBasicConsumeMessage " + Utils::anyToString(i));
		return returnValue->bVal;
	}

	void testBasicCancel() {
		tVariant* params = new tVariant[0];

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicCancel, params, sizeof(params));

		assertTrue(result == true, "testBasicCancel");
	}

	void testBasicAck(uint64_t messageTag) {
		tVariant* params = new tVariant[1];
		params[0].ullVal = messageTag;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicAck, params, sizeof(params));

		assertTrue(result == true, "testBasicAck");
	}

	void testBasicReject(uint64_t messageTag) {
		tVariant* params = new tVariant[1];
		params[0].ullVal = messageTag;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicReject, params, sizeof(params));

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
