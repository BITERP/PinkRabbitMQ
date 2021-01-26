#pragma once
#include <string>
#include <types.h>
#include <iostream>
#include <map>
#include "MessageObject.cpp"
#include <vector>
#include <queue> 
#include <thread>
#include "ThreadSafeQueue.cpp"


class SimplePocoHandler;
namespace AMQP { class Connection; class Channel; class Table; }

class RabbitMQClient {
public:
	RabbitMQClient(const RabbitMQClient &) = delete;
	RabbitMQClient(const RabbitMQClient &&) = delete;
	RabbitMQClient & operator = (const RabbitMQClient&) = delete;
	RabbitMQClient& operator = (const RabbitMQClient&&) = delete;
	RabbitMQClient();
	~RabbitMQClient();
	void setMsgProp(int prop, const std::string& val);
	std::string getMsgProp(int propIndex);
	bool connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost, bool ssl);
	const WCHAR_T* getLastError() noexcept;
	bool basicPublish(std::string& exchange, std::string& routingKey, std::string& message, bool persistent, const std::string& propsJson);
	bool basicAck(const std::uint64_t& messageTag);
	bool basicReject(const std::uint64_t& messageTag);
	bool declareExchange(const std::string& name, const std::string& type, bool mustExists, bool durable, bool autodelete, const std::string& propsJson);
	bool deleteExchange(const std::string& name, bool ifunused);
	std::string declareQueue(const std::string& name, bool onlyCheckIfExists, bool save, bool autodelete, uint16_t maxPriority, const std::string& propsJson);
	bool deleteQueue(const std::string& name, bool onlyIfIdle, bool onlyIfEmpty);
	bool bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey, const std::string& propsJson);
	bool unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	std::string basicConsume(const std::string& queue, int selectSize);
	bool basicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, uint16_t timeout);
	bool basicCancel();
	bool setPriority(int _priority);
	int getPriority();
	std::string getRoutingKey();
	void updateLastError(const char* text);

private:
	AMQP::Channel* openChannel();
	void newConnection(const std::string& login, const std::string& pwd, const std::string& vhost);
	void closeConnection();
	void fillHeadersFromJson(AMQP::Table& headers, const std::string& propsJson);

	std::wstring LAST_ERROR;
	// Transiting properties
	const int CORRELATION_ID = 1;
	const int TYPE_NAME = 2;
	const int MESSAGE_ID = 3;
	const int APP_ID = 4;
	const int CONTENT_ENCODING = 5;
	const int CONTENT_TYPE = 6;
	const int USER_ID = 7;
	const int CLUSTER_ID = 8;
	const int EXPIRATION = 9;
	const int REPLY_TO = 10;
	int priority = 0;
	std::string routingKey = "";
	//
	std::unique_ptr<SimplePocoHandler> handler;
	AMQP::Connection* connection;
	std::unique_ptr<AMQP::Channel> channel;
	std::unique_ptr<AMQP::Channel> publChannel;

	std::queue<std::thread> threadPool;
	std::string consQueue;
	ThreadSafeQueue<MessageObject> readQueue;
	std::map<int, std::string> msgProps;
};