#pragma once
#include <string>
#include <types.h>
#include <iostream>
#include <map>
class SimplePocoHandler;
namespace AMQP { class Connection; class Channel; }

class RabbitMQClient {
public:
	RabbitMQClient(const RabbitMQClient &) = delete;
	RabbitMQClient(const RabbitMQClient &&) = delete;
	RabbitMQClient & operator = (const RabbitMQClient&) = delete;
	RabbitMQClient& operator = (const RabbitMQClient&&) = delete;
	RabbitMQClient() = default;
	~RabbitMQClient();
	void setMsgProp(int prop, const std::string& val);
	std::string getMsgProp(int propIndex);
	bool connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost);
	WCHAR_T* getLastError();
	bool basicPublish(std::string& exchange, std::string& routingKey, std::string& message);
	bool basicAck();
	bool basicReject();
	bool declareExchange(const std::string& name, const std::string& type, bool mustExists, bool durable, bool autodelete);
	bool deleteExchange(const std::string& name, bool ifunused);
	std::string declareQueue(const std::string& name, bool onlyCheckIfExists, bool save, bool exclusive, bool autodelete);
	bool deleteQueue(const std::string& name, bool onlyIfIdle, bool onlyIfEmpty);
	bool bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	bool unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	std::string basicConsume(const std::string& queue);
	bool basicConsumeMessage(std::string& outdata, uint16_t timeout);
	bool basicCancel();
	void updateLastError(const char* text);
private:
	AMQP::Channel* openChannel();
	wchar_t* LAST_ERROR = L"";
	const int CORRELATION_ID = 1;
	const int TYPE_NAME = 2;
	const int MESSAGE_ID = 3;
	SimplePocoHandler* handler;
	AMQP::Connection* connection;
	AMQP::Channel* consChannel;

	std::string consQueue;
	uint64_t messageTag;

	std::map<int, std::string> msgProps;
};