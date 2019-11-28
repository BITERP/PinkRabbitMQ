#include <iostream>
#include "SimplePocoHandler.h"
#include "RabbitMQClient.h"
#include "Utils.h"
#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>
#include <thread>

bool RabbitMQClient::connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost)
{

	updateLastError("");
	if (host.empty() || port == 0) {
		updateLastError("Wrong host or port");
		return false;
	}
	bool connected = false;
	try
	{
		handler = new SimplePocoHandler(host, port);
		connection = new AMQP::Connection(handler, AMQP::Login(login, pwd), vhost);
		
		channel = new AMQP::Channel(connection);
		channel->onReady([this]()
		{
			handler->quit();
		});
		handler->loop();

		threadPool.push(new std::thread(SimplePocoHandler::loopThread, handler));
		
		connected = true;
	}
	catch (const Poco::TimeoutException& ex)
	{
		updateLastError(ex.what());
	}
	catch (const Poco::Net::NetException& ex)
	{
		updateLastError(ex.what());
	}
	return connected;
}

bool RabbitMQClient::declareExchange(const std::string& name, const std::string& type, bool onlyCheckIfExists, bool durable, bool autodelete) {

	AMQP::ExchangeType topic = AMQP::ExchangeType::topic;
	if (type == "topic") {
		topic = AMQP::ExchangeType::topic;
	}
	else if (type == "fanout") {
		topic = AMQP::ExchangeType::fanout;
	}
	else if (type == "direct") {
		topic = AMQP::ExchangeType::direct;
	}
	else {
		updateLastError("Exchange type not supported!");
		return false;
	}

	ThreadSafeQueue<bool> looper(1);

	channel->declareExchange(name, topic, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0))
		.onSuccess([&looper, this]()
	{
		looper.push(true);
	})
		.onError([&looper, this](const char* message)
	{
		updateLastError(message);
		looper.push(false);
	});

	return looper.pop();
}

bool RabbitMQClient::deleteExchange(const std::string& name, bool ifunused) {

	updateLastError("");

	ThreadSafeQueue<bool> looper(1);

	channel->removeExchange(name, (ifunused ? AMQP::ifunused : 0))
		.onSuccess([&looper, this]()
	{
		looper.push(true);
	})
		.onError([&looper, this](const char* message)
	{
		updateLastError(message);
		looper.push(false);
	});

	return looper.pop();
}

std::string RabbitMQClient::declareQueue(const std::string& name, bool onlyCheckIfExists, bool durable, bool autodelete, uint16_t maxPriority) {

	updateLastError("");

	ThreadSafeQueue<bool> looper(1);

	AMQP::Table args = AMQP::Table();
	if (maxPriority != 0) {
		args.set("x-max-priority", maxPriority);
	}

	channel->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0), args)
		.onSuccess([&looper, this]()
	{
		looper.push(true);
	})
		.onError([&looper, this](const char* message)
	{
		updateLastError(message);
		looper.push(false);
	});

	bool resut = looper.pop();

	return name;
}

bool RabbitMQClient::deleteQueue(const std::string& name, bool ifunused, bool ifempty) {

	updateLastError("");
	ThreadSafeQueue<bool> looper(1);

	channel->removeQueue(name, (ifunused ? AMQP::ifunused : 0) | (ifempty ? AMQP::ifempty : 0))
		.onSuccess([&looper, this]()
	{
		looper.push(true);
	})
		.onError([&looper, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		looper.push(false);
	});

	return looper.pop();
}

bool RabbitMQClient::bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

	updateLastError("");
	ThreadSafeQueue<bool> looper(1);

	channel->bindQueue(exchange, queue, routingKey)
		.onSuccess([&looper, this]()
	{
		looper.push(true);
	})
		.onError([&looper, this](const char* message)
	{
		updateLastError(message);
		looper.push(false);
	});

	return looper.pop();
}

bool RabbitMQClient::unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

	updateLastError("");
	ThreadSafeQueue<bool> looper(1);

	channel->unbindQueue(exchange, queue, routingKey)
		.onSuccess([&looper, this]()
	{
		looper.push(true);
	})
		.onError([&looper, this](const char* message)
	{
		updateLastError(message);
		looper.push(false);
	});

	return looper.pop();
}


void RabbitMQClient::setMsgProp(int propNum, const std::string& val) {
	msgProps[propNum] = val;
}

std::string RabbitMQClient::getMsgProp(int propNum) {
	return msgProps[propNum];
}

bool RabbitMQClient::basicPublish(std::string& exchange, std::string& routingKey, std::string& message) {

	updateLastError("");

	if (connection == nullptr) {
		updateLastError("Connection is not established! Use the method Connect() first");
		return false;
	}

	std::mutex mutex;
	std::unique_lock<std::mutex> lock(mutex);
	std::condition_variable cvPop;

	AMQP::Channel publChannel(connection);

	publChannel.onReady([&cvPop, &message, &exchange, &publChannel, &routingKey, this]()
	{
		AMQP::Envelope envelope(message.c_str(), strlen(message.c_str()));
		if (!msgProps[CORRELATION_ID].empty()) envelope.setCorrelationID(msgProps[CORRELATION_ID]);
		if (!msgProps[MESSAGE_ID].empty()) envelope.setMessageID(msgProps[MESSAGE_ID]);
		if (!msgProps[TYPE_NAME].empty()) envelope.setTypeName(msgProps[TYPE_NAME]);
		if (!msgProps[APP_ID].empty()) envelope.setAppID(msgProps[APP_ID]);
		if (!msgProps[CONTENT_ENCODING].empty()) envelope.setContentEncoding(msgProps[CONTENT_ENCODING]);
		if (!msgProps[CONTENT_TYPE].empty()) envelope.setContentType(msgProps[CONTENT_TYPE]);
		if (!msgProps[USER_ID].empty()) envelope.setUserID(msgProps[USER_ID]);
		if (!msgProps[CLUSTER_ID].empty()) envelope.setClusterID(msgProps[CLUSTER_ID]);
		if (!msgProps[EXPIRATION].empty()) envelope.setExpiration(msgProps[EXPIRATION]);
		if (!msgProps[REPLY_TO].empty()) envelope.setReplyTo(msgProps[REPLY_TO]);
		if (priority != 0) envelope.setPriority(priority);

		publChannel.publish(exchange, routingKey, envelope);
		cvPop.notify_one();
	});

	publChannel.onError([&cvPop, this](const char* messageErr)
	{
		updateLastError(messageErr);
		handler->quit();
		cvPop.notify_one();
	});

	cvPop.wait(lock);
	return true;
}

std::string RabbitMQClient::basicConsume(const std::string& queue, const int _selectSize) {

	if (connection == nullptr || !connection->usable()) {
		updateLastError("Connection is not ready. Use connect() method to initialize new connection.");
		return "";
	}

	selectSize = _selectSize;

	channel->setQos(_selectSize, true);
	updateLastError("");

	consQueue = queue;

	channel->consume(consQueue)
		.onMessage([this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
	{
		MessageObject* msgOb = new MessageObject();
		msgOb->body.assign(message.body(), message.bodySize());
		msgOb->msgProps[CORRELATION_ID] = message.correlationID();
		msgOb->msgProps[TYPE_NAME] = message.typeName();
		msgOb->msgProps[MESSAGE_ID] = message.messageID();
		msgOb->msgProps[APP_ID] = message.appID();
		msgOb->msgProps[CONTENT_ENCODING] = message.contentEncoding();
		msgOb->msgProps[CONTENT_TYPE] = message.contentType();
		msgOb->msgProps[USER_ID] = message.userID();
		msgOb->msgProps[CLUSTER_ID] = message.clusterID();
		msgOb->msgProps[EXPIRATION] = message.expiration();
		msgOb->msgProps[REPLY_TO] = message.replyTo();
		msgOb->messageTag = deliveryTag;
		msgOb->priority = message.priority();

		readQueue->push(msgOb);
	})
		.onError([this](const char* message)
	{
		updateLastError(message);
	});

	return "";
}


bool RabbitMQClient::basicConsumeMessage(std::string& outdata, uint16_t timeout) {

	updateLastError("");

	std::chrono::milliseconds timeoutSec{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutSec;
	while (!readQueue->empty() || (end - std::chrono::system_clock::now()).count() > 0) {
		if (!readQueue->empty()) {
			MessageObject* read = readQueue->front();
			readQueue->pop();
			outdata = read->body;
			msgProps[CORRELATION_ID] = read->msgProps[CORRELATION_ID];
			msgProps[TYPE_NAME] = read->msgProps[TYPE_NAME];
			msgProps[MESSAGE_ID] = read->msgProps[MESSAGE_ID];
			msgProps[APP_ID] = read->msgProps[APP_ID];
			msgProps[CONTENT_ENCODING] = read->msgProps[CONTENT_ENCODING];
			msgProps[CONTENT_TYPE] = read->msgProps[CONTENT_TYPE];
			msgProps[USER_ID] = read->msgProps[USER_ID];
			msgProps[CLUSTER_ID] = read->msgProps[CLUSTER_ID];
			msgProps[EXPIRATION] = read->msgProps[EXPIRATION];
			msgProps[REPLY_TO] = read->msgProps[REPLY_TO];
			priority = read->priority;

			confirmQueue->push(read);
			return true;
		}
	}

	return false;
}

bool RabbitMQClient::basicAck() {

	if (confirmQueue->empty()) {
		updateLastError("There is no message in queue to confirm!");
		return false;
	}

	updateLastError("");

	MessageObject* read = confirmQueue->front();
	confirmQueue->pop();
	channel->ack(read->messageTag);
	delete read;

	return true;
}

bool RabbitMQClient::basicReject() {

	if (confirmQueue->empty()) {
		updateLastError("There is no message in queue to reject!");
		return false;
	}
	updateLastError("");

	MessageObject* read = confirmQueue->front();
	confirmQueue->pop();
	channel->reject(read->messageTag);
	delete read;

	return true;
}

bool RabbitMQClient::basicCancel() {
	
	return true;
}

bool RabbitMQClient::setPriority(int _priority) {
	priority = _priority;
	return true;
}

int RabbitMQClient::getPriority() {
	return priority;
}

WCHAR_T* RabbitMQClient::getLastError() noexcept
{
	return LAST_ERROR;
}

void RabbitMQClient::updateLastError(const char* text) {
	LAST_ERROR = new wchar_t[strlen(text) + 1];
	Utils::convetToWChar(LAST_ERROR, text);
}

RabbitMQClient::~RabbitMQClient() {

	handler->quitRead();
	for (int i = 0; i < threadPool.size(); i++) {
		std::thread* curr = threadPool.front();
		curr->join();
		threadPool.pop();
	}

	while (!readQueue->empty()) {
		MessageObject* msgOb = readQueue->front();
		readQueue->pop();
		delete msgOb;
	}
	assert(readQueue->empty());

	while (!confirmQueue->empty()) {
		MessageObject* msgOb = confirmQueue->front();
		confirmQueue->pop();
		delete msgOb;
	}
	assert(confirmQueue->empty());

	delete readQueue;
	delete confirmQueue;

	if (connection != nullptr) {
		delete connection;
	}
	if (handler != nullptr) {
		delete handler;
	}
}
