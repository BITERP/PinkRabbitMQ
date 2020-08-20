#include <iostream>
#include "SimplePocoHandler.h"
#include "RabbitMQClient.h"
#include "Utils.h"
#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>
#include <thread>
#include "AuthException.cpp"
#include "ThreadLooper.cpp"

bool RabbitMQClient::connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost)
{
	updateLastError("");
	bool connected = false;
	try
	{
		handler = new SimplePocoHandler(host, port);
		newConnection(login, pwd, vhost);

		channel = new AMQP::Channel(connection);

		connected = true;
	}
	catch (const Poco::TimeoutException & ex)
	{
		updateLastError(ex.what());
	}
	catch (const AuthException & ex)
	{
		updateLastError(ex.what());
	}
	catch (const Poco::Net::NetException & ex)
	{
		updateLastError(ex.what());
	}
	return connected;
}

void RabbitMQClient::newConnection(const std::string& login, const std::string& pwd, const std::string& vhost) {

	connection = new AMQP::Connection(handler, AMQP::Login(login, pwd), vhost);

	const uint16_t timeout = 5000;
	std::chrono::milliseconds timeoutMs{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutMs;
	while (connection->waiting() && (end - std::chrono::system_clock::now()).count() > 0) {
		handler->loopIteration();
	}
	if (!connection->ready()) {
		throw AuthException();
	}
}

AMQP::Channel* RabbitMQClient::openChannel() {
	if (connection == nullptr) {
		updateLastError("Connection is not established! Use the method Connect() first");
		return nullptr;
	}
	AMQP::Channel* channelLoc = new AMQP::Channel(connection);
	channelLoc->onReady([this]()
	{
		handler->quit();
	});

	channelLoc->onError([this](const char* message)
	{
		updateLastError(message);
		handler->quit();
	});
	handler->loop();
	return channelLoc;
}

bool RabbitMQClient::declareExchange(const std::string& name, const std::string& type, bool onlyCheckIfExists, bool durable, bool autodelete) {

	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return false;
	}

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
		channelLoc->close();
		delete channelLoc;
		return false;
	}

	bool result = true;

	channelLoc->declareExchange(name, topic, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0))
		.onSuccess([this]()
	{
		handler->quit();
	})
		.onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();
	channelLoc->close();
	delete channelLoc;

	return result;
}

bool RabbitMQClient::deleteExchange(const std::string& name, bool ifunused) {

	updateLastError("");
	bool result = true;

	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return false;
	}

	channelLoc->removeExchange(name, (ifunused ? AMQP::ifunused : 0))
		.onSuccess([this]()
	{
		handler->quit();
	})
		.onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();
	channelLoc->close();
	delete channelLoc;

	return result;
}

std::string RabbitMQClient::declareQueue(const std::string& name, bool onlyCheckIfExists, bool durable, bool autodelete, uint16_t maxPriority) {

	updateLastError("");

	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return "";
	}

	AMQP::Table args = AMQP::Table();
	if (maxPriority != 0) {
		args.set("x-max-priority", maxPriority);
	}

	channel->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0), args)
		.onSuccess([this]()
	{
		handler->quit();

	})
		.onError([this](const char* message)
	{
		updateLastError(message);
		handler->quit();
	});

	handler->loop();
	channelLoc->close();
	delete channelLoc;

	return name;
}

bool RabbitMQClient::deleteQueue(const std::string& name, bool ifunused, bool ifempty) {

	updateLastError("");
	bool result = true;
	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return false;
	}

	channelLoc->removeQueue(name, (ifunused ? AMQP::ifunused : 0) | (ifempty ? AMQP::ifempty : 0))
		.onSuccess([this]()
	{
		handler->quit();
	})
		.onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();
	channelLoc->close();
	delete channelLoc;

	return result;
}

bool RabbitMQClient::bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

	updateLastError("");
	bool result = true;

	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return false;
	}

	channelLoc->bindQueue(exchange, queue, routingKey)
		.onSuccess([this]()
	{
		handler->quit();
	})
		.onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();
	channelLoc->close();
	delete channelLoc;

	return result;
}

bool RabbitMQClient::unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

	updateLastError("");
	bool result = true;

	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return false;
	}

	channelLoc->unbindQueue(exchange, queue, routingKey)
		.onSuccess([this]()
	{
		handler->quit();
	})
		.onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();
	channelLoc->close();
	delete channelLoc;

	return result;
}


void RabbitMQClient::setMsgProp(int propNum, const std::string& val) {
	msgProps[propNum] = val;
}

std::string RabbitMQClient::getMsgProp(int propNum) {
	return msgProps[propNum];
}

bool RabbitMQClient::basicPublish(std::string& exchange, std::string& routingKey, std::string& message, bool persistent) {

	updateLastError("");

	if (connection == nullptr) {
		updateLastError("Connection is not established! Use the method Connect() first");
		return false;
	}

	bool result = true;

	AMQP::Channel publChannel(connection);

	publChannel.onReady([&message, &persistent, &exchange, &publChannel, &routingKey, this]()
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
		if (persistent) { envelope.setDeliveryMode(2); }
		publChannel.publish(exchange, routingKey, envelope);
		handler->quit();
	});

	publChannel.onError([&result, this](const char* messageErr)
	{
		updateLastError(messageErr);
		handler->quit();
		result = false;
	});
	handler->loop();
	publChannel.close();
	return result;
}

std::string RabbitMQClient::basicConsume(const std::string& queue, const int _selectSize) {

	if (connection == nullptr || !connection->usable()) {
		updateLastError("Connection is not ready. Use connect() method to initialize new connection.");
		return "";
	}

	channel->onReady([this]()
	{
		handler->quit();
	});
	channel->onError([this](const char* message)
	{
		updateLastError(message);
		handler->quitRead();
		handler->quit();
	});

	handler->loop();
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

	for (int i = 0; i < 1; i++) {
		threadPool.push(new std::thread(SimplePocoHandler::loopThread, handler));
	}
	return "";
}


bool RabbitMQClient::basicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, uint16_t timeout) {

	updateLastError("");

	std::chrono::milliseconds timeoutSec{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutSec;
	while (!readQueue->empty() || (end - std::chrono::system_clock::now()).count() > 0) {
		if (!readQueue->empty()) {
			MessageObject* read;
			readQueue->pop(read);

			outdata = read->body;
			outMessageTag = read->messageTag;

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

			delete read;

			return true;
		}
	}

	return false;
}

bool RabbitMQClient::basicAck(const std::uint64_t& messageTag) {

	if (messageTag == 0) {
		updateLastError("Message tag cannot be empty!");
		return false;
	}

	updateLastError("");

	channel->ack(messageTag);

	return true;
}

bool RabbitMQClient::basicReject(const std::uint64_t& messageTag) {

	if (messageTag == 0) {
		updateLastError("Message tag cannot be empty!");
		return false;
	}
	updateLastError("");

	channel->reject(messageTag);

	return true;
}

bool RabbitMQClient::basicCancel() {


	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	handler->quitRead();

	readQueue->close();

	MessageObject* msgOb;
	ThreadSafeQueue<MessageObject*>::QueueResult result;
	while ((result = readQueue->pop(msgOb)) != ThreadSafeQueue<MessageObject*>::CLOSED) {
		delete msgOb;
	};

	for (int i = 0; i < threadPool.size(); i++) {
		std::thread* curr = threadPool.front();
		curr->join();
		threadPool.pop();
	}
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
	// Order below need to be kept
	if (connection != nullptr) {
		connection->close();
	}
	if (handler != nullptr) {
		handler->quitRead();
	}
	for (int i = 0; i < threadPool.size(); i++) {
		std::thread* curr = threadPool.front();
		curr->join();
		threadPool.pop();
	}

	while (!readQueue->empty()) {
		MessageObject* msgOb;
		readQueue->pop(msgOb);
		delete msgOb;
	}
	assert(readQueue->empty());
	delete readQueue;
	// 

	if (connection != nullptr) {
		delete connection;
	}
	if (handler != nullptr) {
		delete handler;
	}
}