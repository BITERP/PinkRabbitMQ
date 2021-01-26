#include <string>
#include "src/AddInNative.h"
#include "src/ConversionWchar.h"
#include "src/Utils.h"
#include <fstream>

class RabbitMQClientTest
{
public:

	explicit RabbitMQClientTest(std::string &configFile):config(configFile)
	{
		testMessage;
		std::ifstream myReadFile;
		myReadFile.open(config.messageFile);
		while (myReadFile && !myReadFile.eof()) {
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

		tVariant params[6];
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

	void testSendMessage() {

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
	
	void testSSL(){
		if (!config.sslport) {
			return;
		}
	    std::cout << "SSLTest Start" << std::endl;
		native = new AddInNative();
		native->enableDebugMode();
		
		testConnect(true);
		testDeclareExchange();
		testDeclareQueue();
		testBindQueue();
		testBasicPublish(L"Hello", 5);
	    
	    delete native;
	    std::cout << "SSLTest End" << std::endl;
	}


	void testDefParams() {
		native = new AddInNative();
		native->enableDebugMode();
		checkDefParam(AddInNative::Methods::eMethConnect, 5);
		checkDefParam(AddInNative::Methods::eMethDeclareExchange, 5);
		checkDefParam(AddInNative::Methods::eMethDeclareQueue, 5);
		checkDefParam(AddInNative::Methods::eMethBindQueue, 3);
		checkDefParam(AddInNative::Methods::eMethBasicPublish, 5);
		delete native;
	}

	void testPublishFail() {
		native = new AddInNative();
		native->enableDebugMode();
		testConnect();

		tVariant params[6];
		WcharWrapper wQueueExchange(L"BadExchange");
		params[0].pwstrVal = wQueueExchange;
		params[1].pwstrVal = wQueueExchange;
		WcharWrapper wMessage(L"OhNO");
		params[2].pwstrVal = wMessage;
		params[3].uintVal = 0;
		params[4].bVal = false;
		params[5].pwstrVal = nullptr;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicPublish, params, 6);
		assertTrue(!result, "Publish failed test");
		//native->CallAsFunc(AddInNative::Methods::eMethGetLastError, params, nullptr, 0);

		delete native;
	}

private:

	void checkDefParam(int methId, int nondefParams) {
		int params = native->GetNParams(methId);
		for (int i = 0; i < params; i++) {
			tVariant p;
			p.vt = VTYPE_ILLEGAL;
			bool res = native->GetParamDefValue(methId, i, &p);
			std::string methParam = "meth: " + Utils::anyToString(methId) + " param: " + Utils::anyToString(i);
			assertTrue(res == (i >= nondefParams), "Default param wrong result in "+methParam);
			assertTrue(!res || p.vt != VTYPE_EMPTY, "VT_EMPTY def param in "+methParam);
		}
	}

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

	void testConnect(bool ssl = false) {
		tVariant params[7];

		WcharWrapper wHost(config.host.c_str());
		params[0].pwstrVal = wHost;
		params[1].uintVal = ssl ? config.sslport : config.port;
		WcharWrapper wUsr(config.usr.c_str());
		params[2].pwstrVal = wUsr;
		WcharWrapper wPwd(config.pwd.c_str());
		params[3].pwstrVal = wPwd;
		WcharWrapper wVhost(config.vhost.c_str());
		params[4].pwstrVal = wVhost;
		params[6].bVal = ssl;

		bool result = native->CallAsProc(AddInNative::Methods::eMethConnect, params, 7);
		assertTrue(result == true, "testConnect");		
	}

	void testDeclareQueue() {
		tVariant returnValue;
		tVariant params[7];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		params[1].bVal = false;
		params[2].bVal = true;
		params[3].bVal = false;
		params[4].bVal = false;
		params[5].uintVal = 0;
		WcharWrapper props(L"");
		params[6].pwstrVal = props;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethDeclareQueue, &returnValue, params, 7);
		assertTrue(result == true, "testDeclareQueue");
	}

	void testDeleteQueue() {
		tVariant params[3];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		params[1].bVal = false;
		params[2].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeleteQueue, params, 3);
		assertTrue(result == true, "testDeleteQueue");
	}

	void testDeclareExchange() {
		tVariant params[6];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		WcharWrapper wExchangeType(L"topic");
		params[1].pwstrVal = wExchangeType;
		params[2].bVal = false;
		params[3].bVal = true;
		params[4].bVal = false;
		WcharWrapper props(L"");
		params[5].pwstrVal = props;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeclareExchange, params, 6);


		assertTrue(result == true, "testDeclareExchange");
	}

	void testDeleteExchange() {
		tVariant params[2];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		params[1].bVal = false;

		bool result = native->CallAsProc(AddInNative::Methods::eMethDeleteExchange, params, 2);
		assertTrue(result == true, "testDeleteExchange");
	}

	void testBindQueue() {
		tVariant params[4];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		params[1].pwstrVal = wQueueExchange;
		WcharWrapper wRoutingKey(L"#");
		params[2].pwstrVal = wRoutingKey;
		WcharWrapper props(L"");
		params[3].pwstrVal = props;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBindQueue, params, 4);
		assertTrue(result == true, "testBindQueue");
	}

	void testBasicPublish(const wchar_t* message, int count) {
		tVariant params[6];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		params[1].pwstrVal = wQueueExchange;
		WcharWrapper wMessage(message);
		params[2].pwstrVal = wMessage;
		params[3].uintVal = 0;
		params[4].bVal = false;
		WcharWrapper props(L"");
		params[5].pwstrVal = props;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicPublish, params, 6);

		if (!result) {
			native->CallAsFunc(AddInNative::Methods::eMethGetLastError, params, nullptr, 0);
		}
	
		assertTrue(result == true, "testBasicPublish " + Utils::anyToString(count));
	}

	void testSetProps(int propNum) {

		tVariant propVar;
		propVar.pstrVal = "test_prop";
		bool setResult = native->SetPropVal(propNum, &propVar);

		tVariant propVarRet;
		bool getResult = native->GetPropVal(propNum, &propVarRet);

		assertTrue(getResult && setResult, "testSetProps");
	}

	void testBasicConsume() {
		
		tVariant returnValue;
		tVariant params[5];

		WcharWrapper wQueueExchange(config.queueExchange.c_str());
		params[0].pwstrVal = wQueueExchange;
		params[4].uintVal= 100;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethBasicConsume, &returnValue, params, 5);

		assertTrue(result, "testBasicConsume");
	}

	uint64_t testBasicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, int i) {
		
		tVariant returnValue;
		tVariant params[4] = {};

		params[3].uintVal = 3000;

		bool result = native->CallAsFunc(AddInNative::Methods::eMethBasicConsumeMessage, &returnValue, params, 4);

		outdata = params[1].pstrVal;
		outMessageTag = params[2].ullVal;

		if (params[1].pstrVal)
			delete[] params[1].pstrVal;

		assertTrue(result, "testBasicConsumeMessage " + Utils::anyToString(i));		
		
		tVariant rkey;
		result = native->CallAsFunc(AddInNative::Methods::eMethGetRoutingKey, &rkey, nullptr, 0);
		std::cerr << "routingkey length: " << rkey.wstrLen << std::endl;

		return returnValue.bVal;
	}

	void testBasicCancel() {
		tVariant params[0];

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicCancel, params, 0);

		assertTrue(result == true, "testBasicCancel");
	}

	void testBasicAck(uint64_t messageTag) {
		tVariant params[1];
		params[0].ullVal = messageTag;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicAck, params, 1);

		assertTrue(result == true, "testBasicAck");
	}

	void testBasicReject(uint64_t messageTag) {
		tVariant params[1];
		params[0].ullVal = messageTag;

		bool result = native->CallAsProc(AddInNative::Methods::eMethBasicReject, params, 1);

		assertTrue(result == true, "testBasicReject");
	}

public:

	struct Config{
		Config(std::string &filename) {
			if (!filename.length()) {
				return;
			}
			std::ifstream file(filename);
			messageFile = readString(file);
			host = readWString(file);
			port = readInt(file);
			usr = readWString(file);
			pwd = readWString(file);
			vhost = readWString(file);
			queueExchange = readWString(file);
			sslport = readInt(file);
		}

		std::string readString(std::ifstream &file) {
			std::string s;
			getline(file, s);
			return s;
		}

		std::wstring readWString(std::ifstream &file) {
			return Utils::stringToWs(readString(file));
		}

		int readInt(std::ifstream &file) {
			return std::stoi(readString(file));
		}
		
		std::string messageFile = "/home/rkudakov/projects/PinkRabbitMQLinux/src/testMessage560kb.txt";
		std::wstring host = L"devdevopsrmq.bit-erp.loc";
		int port = 5672;
		std::wstring usr = L"rkudakov_devops";
		std::wstring pwd = L"rkudakov_devops";
		std::wstring vhost = L"rkudakov_devops";
		std::wstring queueExchange = L"unit_test_linux";
		int sslport = 0;
	};


private:
	Config config;
	AddInNative* native;
	std::string testMessage;

	void assertTrue(bool condition, std::string msg) {
		if (!condition) {
			std::cerr << " TEST FAILED: " << msg << std::endl;
		}
		assert(condition);
		
		auto now = std::chrono::system_clock::now();
		std::time_t end_time = std::chrono::system_clock::to_time_t(now);
		
		std::cout << std::ctime(&end_time) << " TEST PASSED: " << msg << std::endl;
	};

};
