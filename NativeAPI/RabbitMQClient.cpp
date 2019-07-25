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

		AMQP::Channel testChannel(connection);

		testChannel.onReady([&connected, this]()
		{
			connected = true;
			handler->quit();
		});

		testChannel.onError([this](const char* message)
		{
			updateLastError(message);
			handler->quit();
		});
		handler->loop();
		testChannel.close();
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
	AMQP::Channel* channel = new AMQP::Channel (connection);
	channel->onReady([this]()
	{
		handler->quit();
	});

	channel->onError([this](const char* message)
	{
		updateLastError(message);
		handler->quit();
	});
	handler->loop();
	return channel;
}

bool RabbitMQClient::declareExchange(const std::string& name, const std::string& type, bool onlyCheckIfExists, bool durable, bool autodelete) {

	if (onlyCheckIfExists) {
		updateLastError("Checking if the exchange exist not supported!");
		return false;
	}

	AMQP::Channel* channel = openChannel();
	if (channel == nullptr) {
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
		return false;
	}

	bool result = true;

	channel->declareExchange(name, topic, (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0))
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
	channel->close();
	delete channel;

	return result;
}

bool RabbitMQClient::deleteExchange(const std::string& name, bool ifunused) {

	updateLastError("");
	bool result = true;
	
	AMQP::Channel* channel = openChannel();
	if (channel == nullptr) {
		return false;
	}

	channel->removeExchange(name, (ifunused ? AMQP::ifunused : 0))
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
	channel->close();
	delete channel;

	return result;
}

std::string RabbitMQClient::declareQueue(const std::string& name, bool onlyCheckIfExists, bool durable, bool exclusive, bool autodelete) {

	exclusive = false;
	updateLastError("");

	AMQP::Channel* channel = openChannel();
	if (channel == nullptr) {
		return "";
	}

	channel->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0))
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
	channel->close();
	delete channel;

	return name;
}

bool RabbitMQClient::deleteQueue(const std::string& name, bool ifunused, bool ifempty) {

	updateLastError("");
	bool result = true;
	AMQP::Channel* channel = openChannel();
	if (channel == nullptr) {
		return false;
	}

	channel->removeQueue(name, (ifunused ? AMQP::ifunused : 0) | (ifempty ? AMQP::ifempty : 0))
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
	channel->close();
	delete channel;

	return result;
}

bool RabbitMQClient::bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

	updateLastError("");
	bool result = true;

	AMQP::Channel* channel = openChannel();
	if (channel == nullptr) {
		return false;
	}

	channel->bindQueue(exchange, queue, routingKey)
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
	channel->close();
	delete channel;

	return result;
}

bool RabbitMQClient::unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

	updateLastError("");
	bool result = true;

	AMQP::Channel* channel = openChannel();
	if (channel == nullptr) {
		return false;
	}

	channel->unbindQueue(exchange, queue, routingKey)
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
	channel->close();
	delete channel;

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
		envelope.setCorrelationID(msgProps[CORRELATION_ID]);
		envelope.setMessageID(msgProps[MESSAGE_ID]);
		envelope.setTypeName(msgProps[TYPE_NAME]);

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

std::string RabbitMQClient::basicConsume(const std::string& queue) {

	if (connection == nullptr || !connection->usable()) {
		updateLastError("Connection is not ready. Use connect() method to initialize new connection.");
		return "";
	}

	updateLastError("");

	consQueue = queue;
	consChannel = new AMQP::Channel(connection);
	if (consChannel == nullptr) {
		return false;
	}
	consChannel->onReady([this]()
	{
		handler->quit();
	});

	consChannel->onError([this](const char* message)
	{
		updateLastError(message);
		handler->quit();
	});

	handler->loop();
	return "";
}

bool RabbitMQClient::basicConsumeMessage(std::string& outdata, uint16_t timeout) {

	if (consChannel == nullptr || !consChannel->usable()) {
		updateLastError("Channel with this id not found or has been closed or is not usable!");
		return false;
	}

	updateLastError("");

	bool hasMessage = false;

	consChannel->setQos(1, true);

	if (timeout < 3000) 
	{
		// Admit that only single message may arrive for 3 seconds
		consChannel->get(consQueue).onMessage([&outdata, &hasMessage, this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
		{
			outdata = std::string(message.body(), message.body() + message.bodySize());
			msgProps[CORRELATION_ID] = message.correlationID();
			msgProps[TYPE_NAME] = message.typeName();
			msgProps[MESSAGE_ID] = message.messageID();

			hasMessage = true;
			messageTag = deliveryTag;
			handler->quit();
		})
			.onEmpty([&hasMessage, this]()
		{
			hasMessage = false;
			handler->quit();
		})
			.onError([this](const char* message)
		{
			updateLastError(message);
			handler->quit();
		});

		handler->loop();
	}
	else
	{
		consChannel->consume(consQueue)
			.onMessage([&outdata, &hasMessage, this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
		{
			outdata = std::string(message.body(), message.body() + message.bodySize());
			msgProps[CORRELATION_ID] = message.correlationID();
			msgProps[TYPE_NAME] = message.typeName();
			msgProps[MESSAGE_ID] = message.messageID();

			hasMessage = true;
			messageTag = deliveryTag;
			handler->quit();
		})
			.onError([this](const char* message)
		{
			updateLastError(message);
			handler->quit();
		});

		handler->loop(timeout);
	}

	return hasMessage;
}

bool RabbitMQClient::basicAck() {

	if (consChannel == nullptr || !consChannel->usable()) {
		updateLastError("Channel not found or not usable!");
		return false;
	}

	if (messageTag == 0) {
		updateLastError("There is not message in queue to confirm!");
		return false;
	}

	updateLastError("");
	bool result = true;

	consChannel->onReady([this]() {
		consChannel->ack(messageTag);
		handler->quit();
	});

	consChannel->onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();

	return result;
}

bool RabbitMQClient::basicReject() {

	if (consChannel == nullptr || !consChannel->usable()) {
		updateLastError("Channel not found or not usable!");
		return false;
	}

	if (messageTag == 0) {
		updateLastError("There is no message in queue to reject!");
		return false;
	}

	updateLastError("");
	bool result = true;

	consChannel->onReady([this]() {
		consChannel->reject(messageTag);
		handler->quit();
	});

	consChannel->onError([&result, this](const char* message)
	{
		updateLastError(message);
		handler->quit();
		result = false;
	});

	handler->loop();

	return result;
}

bool RabbitMQClient::basicCancel() {
	if (consChannel == nullptr) {
		updateLastError("Channel not found");
		return false;
	}
	delete consChannel;
	return true;
}

WCHAR_T* RabbitMQClient::getLastError()
{
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