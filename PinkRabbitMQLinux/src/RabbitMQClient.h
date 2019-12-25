#pragma once
#include "event2/event.h"
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <thread>
#include "MessageObject.cpp"
#include "ThreadSafeQueue.cpp"

class RabbitMQClient{
public:
	RabbitMQClient(const RabbitMQClient&) = delete;
	RabbitMQClient(const RabbitMQClient&&) = delete;
	RabbitMQClient& operator = (const RabbitMQClient&) = delete;
	RabbitMQClient& operator = (const RabbitMQClient&&) = delete;
	RabbitMQClient() = default;
	~RabbitMQClient();

	bool connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost);
	std::string declareQueue(const std::string& name, bool onlyCheckIfExists, bool durable, bool autodelete, uint16_t maxPriority);
	bool declareExchange(const std::string& name, const std::string& type, bool mustExists, bool durable, bool autodelete);
	bool deleteExchange(const std::string& name, bool ifunused);
	bool deleteQueue(const std::string& name, bool onlyIfIdle, bool onlyIfEmpty);
	bool bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	bool unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey);
	bool basicPublish(std::string& exchange, std::string& routingKey, std::string& message, bool persistent);
	void setMsgProp(int prop, const std::string& val);
	std::string getMsgProp(int propIndex);
	bool setPriority(int _priority);
	int getPriority();
	std::string basicConsume(const std::string& queue, const int _selectSize);
	bool basicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, uint16_t timeout);
	bool basicAck(const std::uint64_t& messageTag);
	bool basicReject(const std::uint64_t& messageTag);
	bool basicCancel();
	std::string getLastError();
	static void loopThread(event_base* eventLoop);

private:

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

	event_base* eventLoop;
	AMQP::TcpConnection* connection;
	AMQP::LibEventHandler* handler;
	AMQP::TcpChannel* channel;
	std::map<int, std::string> msgProps;
	std::string lastError;

	bool checkConnection();
	AMQP::TcpChannel* openChannel();
	bool checkChannel(AMQP::TcpChannel* _channel);
	std::queue<std::thread*> threadPool;
	ThreadSafeQueue<MessageObject*>* readQueue = new ThreadSafeQueue<MessageObject*>(1);
};