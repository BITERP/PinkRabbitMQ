#include "RabbitMQClient.h"
#include <nlohmann/json.hpp>
#if defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
typedef struct addrinfo AINFO;
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
typedef ADDRINFOA AINFO;
#endif

using json = nlohmann::json;

void RabbitMQClient::connectImpl(Biterp::CallContext& ctx) {
	string host = ctx.stringParamUtf8();
	uint16_t port = ctx.intParam();
	string user = ctx.stringParamUtf8();
	string pwd = ctx.stringParamUtf8();
	string vhost = ctx.stringParamUtf8();
	ctx.skipParam();
	bool ssl = ctx.boolParam();
	int timeout = ctx.intParam();

	if (host.empty()) {
		throw Biterp::Error("Empty hostname not allowed");
	}

	AINFO* _info = nullptr;
	auto code = getaddrinfo(host.c_str(), nullptr, nullptr, &_info);
	if (code) {
		throw Biterp::Error("Wrong hostname: ") << host;
	}
	freeaddrinfo(_info);

	AMQP::Address address(host, port, AMQP::Login(user, pwd), vhost, ssl);

	clear();
	connection.reset(new Connection(address, timeout));
	try {
		connection->connect();
	}
	catch (exception&) {
		connection.reset(nullptr);
		throw;
	}
}


void RabbitMQClient::declareExchangeImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string name = ctx.stringParamUtf8();
	string type = ctx.stringParamUtf8();
	bool onlyCheckIfExists = ctx.boolParam();
	bool durable = ctx.boolParam();
	bool autodelete = ctx.boolParam();
	string propsJson = ctx.stringParamUtf8();

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
		throw Biterp::Error("Exchange type not supported: " + type);
	}

	AMQP::Table args = headersFromJson(propsJson);
	{
		connection->channel()
			->declareExchange(name, topic, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0), args)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
}


void RabbitMQClient::deleteExchangeImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string name = ctx.stringParamUtf8();
	bool ifunused = ctx.boolParam();
	{
		connection->channel()
			->removeExchange(name, (ifunused ? AMQP::ifunused : 0))
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
}

void RabbitMQClient::declareQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string name = ctx.stringParamUtf8();
	bool onlyCheckIfExists = ctx.boolParam();
	bool durable = ctx.boolParam();
	bool exclusive = ctx.boolParam();
	bool autodelete = ctx.boolParam();
	uint16_t maxPriority = ctx.intParam();
	string propsJson = ctx.stringParamUtf8();

	AMQP::Table args = headersFromJson(propsJson);
	if (maxPriority != 0) {
		args.set("x-max-priority", maxPriority);
	}
	{
		connection->channel()
			->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (exclusive ? AMQP::exclusive : 0) | (autodelete ? AMQP::autodelete : 0), args)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
	ctx.setStringResult(u16Converter.from_bytes(name));
}


void RabbitMQClient::deleteQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string name = ctx.stringParamUtf8();
	bool ifunused = ctx.boolParam();
	bool ifempty = ctx.boolParam();
	{
		connection->channel()
			->removeQueue(name, (ifunused ? AMQP::ifunused : 0) | (ifempty ? AMQP::ifempty : 0))
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
}

void RabbitMQClient::bindQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string queue = ctx.stringParamUtf8();
	string exchange = ctx.stringParamUtf8();
	string routingKey = ctx.stringParamUtf8();
	string propsJson = ctx.stringParamUtf8();

	AMQP::Table args = headersFromJson(propsJson);
	{
		connection->channel()
			->bindQueue(exchange, queue, routingKey, args)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
}

void RabbitMQClient::unbindQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string queue = ctx.stringParamUtf8();
	string exchange = ctx.stringParamUtf8();
	string routingKey = ctx.stringParamUtf8();
	{
		connection->channel()
			->unbindQueue(exchange, queue, routingKey)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
}



void RabbitMQClient::basicPublishImpl(Biterp::CallContext& ctx) {
	checkConnection();

	string exchange = ctx.stringParamUtf8();
	string routingKey = ctx.stringParamUtf8();
	string message = ctx.stringParamUtf8();
	ctx.skipParam();
	bool persistent = ctx.boolParam();
	string propsJson = ctx.stringParamUtf8();

	AMQP::Table args = headersFromJson(propsJson);

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
	envelope.setHeaders(headersFromJson(propsJson));
	{
		AMQP::Channel* ch = connection->channel();
		ch->startTransaction();
		ch->publish(exchange, routingKey, envelope);
		ch->commitTransaction()
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
}


void RabbitMQClient::basicConsumeImpl(Biterp::CallContext& ctx) {
	checkConnection();
	string queue = ctx.stringParamUtf8();
	string consumerId = ctx.stringParamUtf8(true);
	bool noconfirm = ctx.boolParam();
	bool exclusive = ctx.boolParam();
	int selectSize = ctx.intParam();
	string propsJson = ctx.stringParamUtf8();

	AMQP::Table args = headersFromJson(propsJson);
	string result;
	{
		AMQP::Channel* channel = connection->readChannel();
		channel->setQos(selectSize);
		channel->consume(queue, consumerId, (noconfirm ? AMQP::noack : 0) | (exclusive ? AMQP::exclusive : 0), args)
			.onSuccess([this, &result](const string& tag)
				{
					result = tag;
					consumers.push_back(tag);
					connection->loopbreak();
				})
			.onMessage([this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
				{
					LOGI("Consume new message arrived");
					MessageObject msgOb;
					msgOb.body.assign(message.body(), message.bodySize());
					msgOb.msgProps[CORRELATION_ID] = message.correlationID();
					msgOb.msgProps[TYPE_NAME] = message.typeName();
					msgOb.msgProps[MESSAGE_ID] = message.messageID();
					msgOb.msgProps[APP_ID] = message.appID();
					msgOb.msgProps[CONTENT_ENCODING] = message.contentEncoding();
					msgOb.msgProps[CONTENT_TYPE] = message.contentType();
					msgOb.msgProps[USER_ID] = message.userID();
					msgOb.msgProps[CLUSTER_ID] = message.clusterID();
					msgOb.msgProps[EXPIRATION] = message.expiration();
					msgOb.msgProps[REPLY_TO] = message.replyTo();
					msgOb.messageTag = deliveryTag;
					msgOb.priority = message.priority();
					msgOb.routingKey = message.routingkey();
					msgOb.headers = message.headers();
					{
						unique_lock<mutex> lock(_mutex);
						LOGI("Consume push message");
						messageQueue.push(msgOb);
						cvDataArrived.notify_all();
					}
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	}
	connection->loop();
	ctx.setStringResult(u16Converter.from_bytes(result));
}


void RabbitMQClient::basicConsumeMessageImpl(Biterp::CallContext& ctx) {
	if (consumers.empty()) {
		throw Biterp::Error("No active consumers");
	}
	inConsume = true;
	ctx.skipParam();
	tVariant* outdata = ctx.skipParam();
	tVariant* outMessageTag = ctx.skipParam();
	int timeout = ctx.intParam();
	ctx.setEmptyResult(outdata);
	ctx.setLongResult(0, outMessageTag);
	{
		unique_lock<mutex> lock(_mutex);
		if (!cvDataArrived.wait_for(lock, chrono::milliseconds(timeout), [&] { return !messageQueue.empty(); })) {
			ctx.setBoolResult(false);
			inConsume = false;
			return;
		}
		if (messageQueue.empty()) {
			inConsume = false;
			throw Biterp::Error("Empty consume message");
		}
		lastMessage = messageQueue.front();
		messageQueue.pop();
	}
	ctx.setStringResult(u16Converter.from_bytes(lastMessage.body), outdata);
	ctx.setLongResult(lastMessage.messageTag, outMessageTag);
	ctx.setBoolResult(true);
	inConsume = false;
}

void RabbitMQClient::clear() {
	if (!consumers.empty() && connection) {
		AMQP::Channel* ch = connection->readChannel();
		ch->startTransaction();
		for (auto& tag : consumers) {
			ch->cancel(tag);
		}	
		ch->commitTransaction()
			.onFinalize([&]() {
				ch->close();
				connection->loopbreak();
			});
		connection->loop();
	}
	consumers.clear();
	unique_lock<mutex> lock(_mutex);
	queue<MessageObject> empty;
	messageQueue.swap(empty);
	if (inConsume) {
		cvDataArrived.notify_all();
		this_thread::sleep_for(chrono::seconds(1));
	}
}

void RabbitMQClient::basicCancelImpl(Biterp::CallContext& ctx) {
	checkConnection();
	clear();
}

void RabbitMQClient::basicAckImpl(Biterp::CallContext& ctx) {
	checkConnection();
	uint64_t tag = ctx.longParam();
	if (tag == 0) {
		throw Biterp::Error("Message tag cannot be empty!");
	}
	connection->readChannel()->ack(tag);
	this_thread::sleep_for(chrono::microseconds(10));
}

void RabbitMQClient::basicRejectImpl(Biterp::CallContext& ctx) {
	checkConnection();
	uint64_t tag = ctx.longParam();
	if (tag == 0) {
		throw Biterp::Error("Message tag cannot be empty!");
	}
	connection->readChannel()->reject(tag);
	this_thread::sleep_for(chrono::microseconds(10));
}

void RabbitMQClient::checkConnection() {
	if (!connection) {
		throw Biterp::Error("Connection is not established! Use the method Connect() first");
	}
}

void RabbitMQClient::sleepNativeImpl(Biterp::CallContext& ctx) {

	uint64_t amount = ctx.longParam();
	std::this_thread::sleep_for(std::chrono::milliseconds(amount));
}

AMQP::Table RabbitMQClient::headersFromJson(const string& propsJson)
{
	AMQP::Table headers;
	if (!propsJson.length()) {
		return headers;
	}
	auto object = json::parse(propsJson);

	for (auto& it : object.items()) {
		auto& value = it.value();
		std::string name = it.key();
		if (value.is_boolean())
		{
			headers.set(name, value.get<bool>());
		}
		else if (value.is_number())
		{
			headers.set(name, value.get<int64_t>());
		}
		else if (value.is_string())
		{
			headers.set(name, value.get<std::string>());
		}
		else
		{
			throw Biterp::Error("Unsupported json type for property " + name);
		}
	}
	return headers;
}

string RabbitMQClient::lastMessageHeaders() {
	AMQP::Table& headersTbl = lastMessage.headers;
	json hdr = json::object();
	for (const std::string& key : headersTbl.keys()) {
		const AMQP::Field& field = headersTbl.get(key);
		if (field.isBoolean()) {
			const AMQP::BooleanSet& boolField = dynamic_cast<const AMQP::BooleanSet&>(field);
			hdr[key] = (bool)boolField.get(0);
		}
		else if (field.isInteger()) {
			hdr[key] = (int64_t)field;
		}
		else if (field.isDecimal()) {
			hdr[key] = (double)field;
		}
		else if (field.isString()) {
			hdr[key] = (const std::string&)field;
		}
	}
	return hdr.dump();
}