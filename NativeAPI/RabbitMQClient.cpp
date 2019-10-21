#include <iostream>
#include "SimplePocoHandler.h"
#include "RabbitMQClient.h"
#include "Utils.h"
#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>


bool RabbitMQClient::connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost)
{
	updateLastError("");
	bool connected = false;
	try
	{
		handler = new SimplePocoHandler(host, port);
		connection = new AMQP::Connection(handler, AMQP::Login(login, pwd), vhost);

		channel = new AMQP::Channel(connection);
		
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

AMQP::Channel* RabbitMQClient::openChannel() {
	if (connection == nullptr) {
		updateLastError("Connection is not established! Use the method Connect() first");
		return nullptr;
	}
	AMQP::Channel* channelLoc = new AMQP::Channel (connection);
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

std::string RabbitMQClient::declareQueue(const std::string& name, bool onlyCheckIfExists, bool durable, bool autodelete) {

	updateLastError("");

	AMQP::Channel* channelLoc = openChannel();
	if (channelLoc == nullptr) {
		return "";
	}

	channelLoc->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0))
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

bool RabbitMQClient::basicPublish(std::string& exchange, std::string& routingKey, std::string& message) {

	updateLastError("");
	bool result = true;
	AMQP::Channel publChannel(connection);

	publChannel.onReady([&message, &exchange, &publChannel, &routingKey, this]()
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

	channel->setQos(100, false);

	channel->onReady([this]()
	{
		handler->quit();
	});
	handler->loop();
	updateLastError("");

	consQueue = queue;

	return "";
}

bool RabbitMQClient::basicConsumeMessage(std::string& outdata, uint16_t timeout) {

	if (channel == nullptr || !channel->usable()) {
		updateLastError("Channel with this id not found or has been closed or is not usable!");
		return false;
	}

	updateLastError("");

	if (readQueue.size() > 0) {
		MessageObject* read = readQueue.front();
		readQueue.pop();
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

		confirmQueue.push(read);
		return true;
	};


	bool hasMessage = false;

	channel->setQos(100, false);
	AMQP::MessageCallback messageCallback = [&timeout , &hasMessage, &outdata, this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
	{
		MessageObject* msgOb = new MessageObject();
		
		msgOb->body = std::string(message.body(), message.body() + message.bodySize());
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
		
		readQueue.push(msgOb);

		handler->quitRead();

		hasMessage = true;
	};

	channel->consume(consQueue)
		.onMessage(messageCallback)
			
		.onError([this](const char* message)
	{
		updateLastError(message);
		handler->quitRead();
	});

	handler->loop(timeout);

	if (hasMessage) {
		MessageObject* read = readQueue.front();
		readQueue.pop();
		outdata = read->body;
		msgProps = read->msgProps;
		confirmQueue.push(read);
	}

	return hasMessage;
}

bool RabbitMQClient::basicAck() {

	if (channel == nullptr || !channel->usable()) {
		updateLastError("Channel not found or not usable!");
		return false;
	}

	if (confirmQueue.size() == 0) {
		updateLastError("There is no message in queue to confirm!");
		return false;
	}

	updateLastError("");
	bool result = false;

	channel->onReady([this, &result]() {
		MessageObject* read = confirmQueue.front();
		confirmQueue.pop();
		channel->ack(read->messageTag);
		read->body;
		delete read;
		handler->quit();
		result = true;
	});
	handler->loop();

	return result;
}

bool RabbitMQClient::basicReject() {

	if (channel == nullptr || !channel->usable()) {
		updateLastError("Channel not found or not usable!");
		return false;
	}

	if (confirmQueue.size() == 0) {
		updateLastError("There is no message in queue to reject!");
		return false;
	}

	updateLastError("");
	bool result = false;

	channel->onReady([this, &result]() {
		MessageObject* read = confirmQueue.front();
		confirmQueue.pop();
		channel->reject(read->messageTag);
		delete read;
		handler->quit();
		result = true;
	});
	handler->loop();

	return result;
}

bool RabbitMQClient::basicCancel() {
	if (channel == nullptr) {
		updateLastError("Channel not found");
		return false;
	}
	return true;
}

WCHAR_T* RabbitMQClient::getLastError() noexcept {
	return LAST_ERROR;
}


void RabbitMQClient::updateLastError(const char* text) {
	LAST_ERROR = new wchar_t[strlen(text) + 1];
	Utils::convetToWChar(LAST_ERROR, text);
}

RabbitMQClient::~RabbitMQClient() {
	if (connection != nullptr) {
		delete connection;
	}
	if (handler != nullptr) {
		delete handler;
	}
}
