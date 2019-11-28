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
	WCHAR_T* getLastError() noexcept;
	bool basicPublish(std::string& exchange, std::string& routingKey, std::string& message);
	bool basicAck();
	bool basicReject();
	bool declareExchange(const std::string& name, const std::string& type, bool mustExists, bool durable, bool autodelete);
	bool deleteExchange(const std::string& name, bool ifunused);
	std::string declareQueue(const std::string& name, bool onlyCheckIfExists, bool save, bool autodelete, uint16_t maxPriority);
	bool deleteQueue(const std::string& name, bool onlyIfIdle, bool onlyIfEmpty);
	bool bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	bool unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	std::string basicConsume(const std::string& queue, int selectSize);
	bool basicConsumeMessage(std::string& outdata, uint16_t timeout);
	void basicConsumeMessageThread(uint16_t timeout);
	bool basicCancel();
	bool setPriority(int _priority);
	int getPriority();
	void updateLastError(const char* text);
private:
	AMQP::Channel* openChannel();
	wchar_t* LAST_ERROR = L"";
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
	int selectSize = 1;
	int priority = 0;
	//
	SimplePocoHandler* handler;
	AMQP::Connection* connection;
	AMQP::Channel* channel;

	std::queue<std::thread*> threadPool;
	std::queue<MessageObject>* readQueue = new std::queue<MessageObject>();
	ThreadSafeQueue<MessageObject>* confirmQueue = new ThreadSafeQueue<MessageObject>(1);
	std::string consQueue;
	std::map<int, std::string> msgProps;
};