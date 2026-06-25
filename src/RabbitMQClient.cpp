#include "RabbitMQClient.h"
#include "Utils.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#if defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>
typedef struct addrinfo AINFO;
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
typedef ADDRINFOA AINFO;
#endif

using json = nlohmann::json;

namespace {

constexpr size_t AMQP_SHORTSTR_MAX = 255;

void validateShortString(const std::string& value, const char* fieldName)
{
	if (value.size() > AMQP_SHORTSTR_MAX) {
		throw Biterp::Error(std::string(fieldName) + " exceeds AMQP limit of 255 bytes");
	}
}

json amqpFieldToJson(const AMQP::Field& field)
{
	if (field.isBoolean()) {
		const AMQP::BooleanSet& boolField = dynamic_cast<const AMQP::BooleanSet&>(field);
		return json(static_cast<bool>(boolField.get(0)));
	}
	if (field.isInteger()) {
		return json(static_cast<int64_t>(field));
	}
	if (field.isDecimal()) {
		return json(static_cast<double>(field));
	}
	if (field.isString()) {
		return json(static_cast<const std::string&>(field));
	}
	if (field.isArray()) {
		json arr = json::array();
		const AMQP::Array& array = field;
		for (uint32_t i = 0; i < array.count(); ++i) {
			arr.push_back(amqpFieldToJson(array.get(static_cast<uint8_t>(i))));
		}
		return arr;
	}
	if (field.isTable()) {
		json object = json::object();
		const AMQP::Table& table = field;
		for (const std::string& key : table.keys()) {
			object[key] = amqpFieldToJson(table.get(key));
		}
		return object;
	}
	if (field.isVoid()) {
		return nullptr;
	}
	return json(nullptr);
}

} // namespace

void RabbitMQClient::connectImpl(Biterp::CallContext& ctx) {
	std::string host = ctx.stringParamUtf8();
	const int portVal = ctx.intParam();
	if (portVal < 0 || portVal > 65535) {
		throw Biterp::Error("Port number out of range");
	}
	const uint16_t port = static_cast<uint16_t>(portVal);
	std::string user = ctx.stringParamUtf8();
	std::string pwd = ctx.stringParamUtf8();
	std::string vhost = ctx.stringParamUtf8();
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
	trChannelConfirmEnabled = false;
	connection.reset(new Connection(address, timeout));
	try {
		connection->setLostCallback([this](const std::string& err) {
			onConnectionLost(err);
		});
		connection->connect();
		connection->channel();
		connection->readChannel();
		connection->loopbreak();
		{
			std::lock_guard<std::mutex> lock(_mutex);
			consumerError.clear();
		}
	}
	catch (std::exception&) {
		connection.reset(nullptr);
		throw;
	}
}


void RabbitMQClient::declareExchangeImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string name = ctx.stringParamUtf8();
	std::string type = ctx.stringParamUtf8();
	bool onlyCheckIfExists = ctx.boolParam();
	bool durable = ctx.boolParam();
	bool autodelete = ctx.boolParam();
	std::string propsJson = ctx.stringParamUtf8();

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
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->declareExchange(name, topic, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0), args)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
}


void RabbitMQClient::deleteExchangeImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string name = ctx.stringParamUtf8();
	bool ifunused = ctx.boolParam();
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->removeExchange(name, (ifunused ? AMQP::ifunused : 0))
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
}

void RabbitMQClient::declareQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string name = ctx.stringParamUtf8();
	bool onlyCheckIfExists = ctx.boolParam();
	bool durable = ctx.boolParam();
	bool exclusive = ctx.boolParam();
	bool autodelete = ctx.boolParam();
	uint16_t maxPriority = ctx.intParam();
	std::string propsJson = ctx.stringParamUtf8();

	AMQP::Table args = headersFromJson(propsJson);
	if (maxPriority != 0) {
		args.set("x-max-priority", maxPriority);
	}
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (exclusive ? AMQP::exclusive : 0) | (autodelete ? AMQP::autodelete : 0), args)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
	ctx.setStringResult(u16Converter.from_bytes(name));
}


void RabbitMQClient::deleteQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string name = ctx.stringParamUtf8();
	bool ifunused = ctx.boolParam();
	bool ifempty = ctx.boolParam();
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->removeQueue(name, (ifunused ? AMQP::ifunused : 0) | (ifempty ? AMQP::ifempty : 0))
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
}

void RabbitMQClient::bindQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string queue = ctx.stringParamUtf8();
	std::string exchange = ctx.stringParamUtf8();
	std::string routingKey = ctx.stringParamUtf8();
	std::string propsJson = ctx.stringParamUtf8();

	validateShortString(queue, "Queue");
	validateShortString(exchange, "Exchange");
	validateShortString(routingKey, "Routing key");

	AMQP::Table args = headersFromJson(propsJson);
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->bindQueue(exchange, queue, routingKey, args)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
}

void RabbitMQClient::unbindQueueImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string queue = ctx.stringParamUtf8();
	std::string exchange = ctx.stringParamUtf8();
	std::string routingKey = ctx.stringParamUtf8();
	validateShortString(queue, "Queue");
	validateShortString(exchange, "Exchange");
	validateShortString(routingKey, "Routing key");
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->unbindQueue(exchange, queue, routingKey)
			.onSuccess([this]()
				{
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
}



void RabbitMQClient::fillEnvelope(AMQP::Envelope& envelope, bool persistent, const AMQP::Table& headers, std::map<int, std::string>& props) {
	if (!props[CORRELATION_ID].empty()) envelope.setCorrelationID(props[CORRELATION_ID]);
	if (!props[MESSAGE_ID].empty()) envelope.setMessageID(props[MESSAGE_ID]);
	if (!props[TYPE_NAME].empty()) envelope.setTypeName(props[TYPE_NAME]);
	if (!props[APP_ID].empty()) envelope.setAppID(props[APP_ID]);
	if (!props[CONTENT_ENCODING].empty()) envelope.setContentEncoding(props[CONTENT_ENCODING]);
	if (!props[CONTENT_TYPE].empty()) envelope.setContentType(props[CONTENT_TYPE]);
	if (!props[USER_ID].empty()) envelope.setUserID(props[USER_ID]);
	if (!props[CLUSTER_ID].empty()) envelope.setClusterID(props[CLUSTER_ID]);
	if (!props[EXPIRATION].empty()) envelope.setExpiration(props[EXPIRATION]);
	if (!props[REPLY_TO].empty()) envelope.setReplyTo(props[REPLY_TO]);
	if (priority != 0) envelope.setPriority(priority);
	if (persistent) { envelope.setDeliveryMode(2); }
	envelope.setHeaders(headers);
}



void RabbitMQClient::basicPublishImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string exchange = ctx.stringParamUtf8();
	std::string routingKey = ctx.stringParamUtf8();
	std::string message = ctx.stringParamUtf8();
	ctx.skipParam();
	bool persistent = ctx.boolParam();
	std::string propsJson = ctx.stringParamUtf8();

	validateShortString(exchange, "Exchange");
	validateShortString(routingKey, "Routing key");

	verifyExchangeExists(exchange);

	AMQP::Envelope envelope(message.data(), message.size());
	fillEnvelope(envelope, persistent, headersFromJson(propsJson), msgProps);
	LoopCallbackGuard loopGuard(*this);
	AMQP::Channel* ch = connection->channel();
	ensurePublisherConfirms(ch);
	connection->withIoLock([&]() {
		ensurePublishReturnHandler(ch);
		waitPublishConfirm(ch);
		ch->publish(exchange, routingKey, envelope, AMQP::mandatory);
	});
	connection->loop();
}


void RabbitMQClient::onConnectionLost(const std::string& error) {
	if (shuttingDown) {
		return;
	}
	deactivateLoopCallbacks();
	std::lock_guard<std::mutex> lock(_mutex);
	consumerError = error.empty() ? "Connection lost" : error;
	cvDataArrived.notify_all();
}

void RabbitMQClient::activateLoopCallbacks() {
	loopCallbackActive = std::make_shared<bool>(true);
}

void RabbitMQClient::deactivateLoopCallbacks() {
	if (loopCallbackActive) {
		*loopCallbackActive = false;
	}
}


void RabbitMQClient::ensurePublisherConfirms(AMQP::Channel* channel) {
	if (trChannelConfirmEnabled) {
		return;
	}
	const auto active = loopCallbackActive;
	connection->withIoLock([&]() {
		channel->confirmSelect()
			.onSuccess([this, active]()
				{
					if (!active || !*active) {
						return;
					}
					trChannelConfirmEnabled = true;
					connection->loopbreak();
				})
			.onError([this, active](const char* message)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak(message);
				});
	});
	connection->loop();
}

void RabbitMQClient::waitPublishConfirm(AMQP::Channel* channel) {
	const auto active = loopCallbackActive;
	connection->withIoLock([&]() {
		channel->confirmSelect()
			.onAck([this, active](uint64_t, bool)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak();
				})
			.onNack([this, active](uint64_t, bool, bool)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak("Publish rejected by broker");
				})
			.onError([this, active](const char* message)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak(message);
				});
	});
}

void RabbitMQClient::waitBatchPublishConfirm(AMQP::Channel* channel, size_t publishCount) {
	batchPublishAckCount = 0;
	batchPublishAckTarget = publishCount;
	const auto active = loopCallbackActive;
	connection->withIoLock([&]() {
		channel->confirmSelect()
			.onAck([this, active](uint64_t, bool multiple)
				{
					if (!active || !*active) {
						return;
					}
					if (multiple) {
						batchPublishAckCount = batchPublishAckTarget;
					}
					else {
						++batchPublishAckCount;
					}
					if (batchPublishAckCount >= batchPublishAckTarget) {
						connection->loopbreak();
					}
				})
			.onNack([this, active](uint64_t, bool, bool)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak("Publish rejected by broker");
				})
			.onError([this, active](const char* message)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak(message);
				});
	});
}

void RabbitMQClient::batchPublishImpl(Biterp::CallContext& ctx) {
	checkConnection();

	constexpr size_t MAX_BATCH_JSON_SIZE = 10 * 1024 * 1024;
	constexpr size_t MAX_BATCH_MESSAGES = 10000;

	std::string exchange = ctx.stringParamUtf8();
	bool persistent = ctx.boolParam();
	const std::string jsonInput = ctx.stringParamUtf8();

	validateShortString(exchange, "Exchange");

	if (jsonInput.size() > MAX_BATCH_JSON_SIZE) {
		throw Biterp::Error("Batch messages JSON input too large");
	}

	json messages;
	try {
		messages = json::parse(jsonInput);
	}
	catch (const json::parse_error& e) {
		throw Biterp::Error(std::string("JSON parse error: ") + e.what());
	}

	if (!messages.is_array()) {
		throw Biterp::Error("Batch messages must be a JSON array");
	}
	if (messages.size() > MAX_BATCH_MESSAGES) {
		throw Biterp::Error("Too many messages in batch");
	}
	if (messages.empty()) {
		return;
	}

	AMQP::Channel* ch = nullptr;
	ch = connection->channel();
	LoopCallbackGuard loopGuard(*this);
	ensurePublisherConfirms(ch);
	connection->withIoLock([&]() {
		ensurePublishReturnHandler(ch);
		waitBatchPublishConfirm(ch, messages.size());

		json empty = json::object();
		for (auto& item : messages) {
			if (!item.is_object()) {
				throw Biterp::Error("Each batch message must be a JSON object");
			}
			if (!item.contains("routingKey") || !item.contains("body")) {
				throw Biterp::Error("Each batch message must have routingKey and body");
			}
			if (!item["routingKey"].is_string() || !item["body"].is_string()) {
				throw Biterp::Error("routingKey and body must be strings");
			}

			std::string routingKey = item["routingKey"].get<std::string>();
			validateShortString(routingKey, "Routing key");
			std::string message = item["body"].get<std::string>();
			AMQP::Envelope envelope(message.data(), message.size());

			json& headers = item.contains("headers") ? item["headers"] : empty;
			json& props = item.contains("properties") ? item["properties"] : empty;
			if (!headers.is_object() || !props.is_object()) {
				throw Biterp::Error("headers and properties must be JSON objects");
			}

			auto propmap = propsFromJson(props);
			fillEnvelope(envelope, persistent, headersFromJson(headers), propmap);
			ch->publish(exchange, routingKey, envelope, AMQP::mandatory);
		}
	});
	connection->loop();
}


void RabbitMQClient::ensurePublishReturnHandler(AMQP::Channel* channel) {
	if (publishReturnHandlerEnabled) {
		return;
	}
	const auto active = loopCallbackActive;
	channel->recall().onReturned([this, active](const AMQP::Message&, int16_t code, const std::string& description)
		{
			if (!active || !*active) {
				return;
			}
			connection->loopbreak("Message returned (code " + std::to_string(code) + "): " + description);
		});
	publishReturnHandlerEnabled = true;
}

void RabbitMQClient::verifyExchangeExists(const std::string& exchange) {
	if (exchange.empty()) {
		return;
	}
	validateShortString(exchange, "Exchange");
	LoopCallbackGuard loopGuard(*this);
	const auto active = loopCallbackActive;
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->declareExchange(exchange, AMQP::ExchangeType::topic, AMQP::passive)
			.onSuccess([this, active]()
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak();
				})
			.onError([this, active](const char* message)
				{
					if (!active || !*active) {
						return;
					}
					connection->loopbreak(message);
				});
	});
	connection->loop();
}

void RabbitMQClient::verifyQueueExists(const std::string& queue) {
	validateShortString(queue, "Queue");
	LoopCallbackGuard loopGuard(*this);
	const auto active = loopCallbackActive;
	AMQP::Channel* ch = connection->readChannel();
	connection->withIoLock([&]() {
		ch->declareQueue(queue, AMQP::passive)
		.onSuccess([this, active](const std::string&, uint32_t, uint32_t)
			{
				if (!active || !*active) {
					return;
				}
				connection->loopbreak();
			})
		.onError([this, active](const char* message)
			{
				if (!active || !*active) {
					return;
				}
				connection->loopbreak(message);
			});
	});
	connection->loop();
}


void RabbitMQClient::basicConsumeImpl(Biterp::CallContext& ctx) {
	checkConnection();
	std::string queue = ctx.stringParamUtf8();
	std::string consumerId = ctx.stringParamUtf8(true);
	bool noconfirm = ctx.boolParam();
	bool exclusive = ctx.boolParam();
	int selectSize = ctx.intParam();
	std::string propsJson = ctx.stringParamUtf8();

	validateShortString(queue, "Queue");
	validateShortString(consumerId, "Consumer tag");

	verifyQueueExists(queue);

	AMQP::Table args = headersFromJson(propsJson, true);
	consumeTagResult.clear();
	LoopCallbackGuard loopGuard(*this);
	const auto active = loopCallbackActive;
	AMQP::Channel* channel = connection->readChannel();
	consumeChannel = channel;
	connection->withIoLock([&]() {
		auto startConsume = [&]() {
			channel->consume(queue, consumerId, (noconfirm ? AMQP::noack : 0) | (exclusive ? AMQP::exclusive : 0), args)
				.onSuccess([this, channel, active](const std::string& tag)
					{
						if (!active || !*active) {
							return;
						}
						consumeTagResult = tag;
						LOGI("Consumer created " + tag);
						{
							std::lock_guard<std::mutex> lock(_mutex);
							consumers.push_back(tag);
							consumerError.clear();
							consumeChannel = channel;
						}
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
							LOGI("Consume push message");
							std::lock_guard<std::mutex> lock(_mutex);
							messageQueue.push(msgOb);
							cvDataArrived.notify_all();
						}
					})
				.onCancelled([this](const std::string &consumer){
						LOGI("Consumer cancelled " + consumer);
						std::lock_guard<std::mutex> lock(_mutex);
						consumers.erase(std::remove_if(consumers.begin(), consumers.end(), [&consumer](std::string& s){return s == consumer;}));
					})
				.onError([this, active](const char* message)
					{
						if (!active || !*active) {
							return;
						}
						{
							std::lock_guard<std::mutex> lock(_mutex);
							consumerError = message;
							LOGE("Consumer error: " + consumerError);
						}
						connection->loopbreak(consumerError);
					});
		};
		if (selectSize > 0) {
			channel->setQos(static_cast<uint16_t>(selectSize), false)
				.onSuccess([this, active, startConsume]()
					{
						if (!active || !*active) {
							return;
						}
						startConsume();
					})
				.onError([this, active](const char* message)
					{
						if (!active || !*active) {
							return;
						}
						connection->loopbreak(message);
					});
		}
		else {
			startConsume();
		}
	});
	connection->loop();
	if (consumeTagResult.empty()) {
		throw Biterp::Error("Consumer was not created");
	}
	ctx.setStringResult(u16Converter.from_bytes(consumeTagResult));
}


void RabbitMQClient::basicConsumeMessageImpl(Biterp::CallContext& ctx) {
	setSkipAddError(true);
	std::string connError;
	if (!connectionReady(connError)) {
		setLastError(u16Converter.from_bytes(connError));
		ctx.setBoolResult(false);
		return;
	}
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (consumers.empty()) {
			setLastError(u16Converter.from_bytes("No active consumers"));
			ctx.setBoolResult(false);
			return;
		}
	}
	ctx.skipParam();
	tVariant* outdata = ctx.skipParam();
	tVariant* outMessageTag = ctx.skipParam();
	int timeout = ctx.intParam();
	ctx.setEmptyResult(outdata);
	ctx.setIntResult(0, outMessageTag);
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (messageQueue.empty()){
			if (!consumerError.empty()){
				setLastError(u16Converter.from_bytes(consumerError));
				ctx.setBoolResult(false);
				return;
			}
			if (connection && connection->isLost()) {
				setLastError(u16Converter.from_bytes("Connection lost"));
				ctx.setBoolResult(false);
				return;
			}
			if (!cvDataArrived.wait_for(lock, std::chrono::milliseconds(timeout), [&] {
				return !messageQueue.empty() || !consumerError.empty() || (connection && connection->isLost());
			})) {
				setLastError(u16Converter.from_bytes("Timeout waiting for message"));
				ctx.setBoolResult(false);
				return;
			}
			if (!consumerError.empty()) {
				setLastError(u16Converter.from_bytes(consumerError));
				ctx.setBoolResult(false);
				return;
			}
			if (connection && connection->isLost()) {
				setLastError(u16Converter.from_bytes("Connection lost"));
				ctx.setBoolResult(false);
				return;
			}
			if (messageQueue.empty()) {
				setLastError(u16Converter.from_bytes("Empty consume message"));
				ctx.setBoolResult(false);
				return;
			}
		}
		lastMessage = messageQueue.front();
		messageQueue.pop();
	}
	try {
		ctx.setStringResult(u16Converter.from_bytes(lastMessage.body), outdata);
	}
	catch (const std::exception& e) {
		setLastError(u16Converter.from_bytes(e.what()));
		ctx.setBoolResult(false);
		return;
	}
	ctx.setIntResult(lastMessage.messageTag, outMessageTag);
	ctx.setBoolResult(true);
}

void RabbitMQClient::cancelAllConsumers() {
	std::vector<std::string> tagsToCancel;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		tagsToCancel = consumers;
	}
	if (!connection || tagsToCancel.empty()) {
		return;
	}
	LoopCallbackGuard loopGuard(*this);
	for (const std::string& tag : tagsToCancel) {
		try {
			AMQP::Channel* ch = connection->readChannel();
			connection->withIoLock([&]() {
				ch->cancel(tag)
					.onSuccess([this](const std::string& cancelledTag)
						{
							LOGI("Consumer cancelled on broker: " + cancelledTag);
							connection->loopbreak();
						})
					.onError([this](const char* message)
						{
							connection->loopbreak(message);
						});
			});
			connection->loop();
		}
		catch (std::exception&) {
			connection->loopbreak();
		}
	}
}

void RabbitMQClient::clear() {
	shuttingDown = true;
	deactivateLoopCallbacks();
	cancelAllConsumers();
	if (connection) {
		connection->loopbreak();
		connection->shutdown();
		connection.reset(nullptr);
	}
	std::lock_guard<std::mutex> lock(_mutex);
	consumers.clear();
	consumeChannel = nullptr;
	consumerError.clear();
	trChannelConfirmEnabled = false;
	publishReturnHandlerEnabled = false;
	std::queue<MessageObject> empty;
	messageQueue.swap(empty);
	cvDataArrived.notify_all();
	shuttingDown = false;
}

void RabbitMQClient::basicCancelImpl(Biterp::CallContext& ctx) {
	checkConnection();
	std::string consumerTag = ctx.stringParamUtf8();

	std::vector<std::string> tagsToCancel;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (consumerTag.empty()) {
			tagsToCancel = consumers;
		}
		else {
			tagsToCancel.push_back(consumerTag);
		}
	}

	if (tagsToCancel.empty()) {
		clear();
		return;
	}

	for (const std::string& tag : tagsToCancel) {
		AMQP::Channel* ch = connection->readChannel();
		connection->withIoLock([&]() {
			ch->cancel(tag)
				.onSuccess([this](const std::string& cancelledTag)
					{
						LOGI("Consumer cancelled on broker: " + cancelledTag);
						connection->loopbreak();
					})
				.onError([this](const char* message)
					{
						connection->loopbreak(message);
					});
		});
		connection->loop();
	}

	{
		std::lock_guard<std::mutex> lock(_mutex);
		std::queue<MessageObject> empty;
		messageQueue.swap(empty);
		if (consumerTag.empty()) {
			consumers.clear();
			consumeChannel = nullptr;
		}
		else {
			consumers.erase(std::remove_if(consumers.begin(), consumers.end(),
				[&consumerTag](const std::string& s) { return s == consumerTag; }), consumers.end());
			if (consumers.empty()) {
				consumeChannel = nullptr;
			}
		}
		cvDataArrived.notify_all();
	}
}

void RabbitMQClient::basicAckImpl(Biterp::CallContext& ctx) {
	checkConnection();
	uint64_t tag = ctx.longParam();
	if (tag == 0) {
		throw Biterp::Error("Message tag cannot be empty!");
	}
	AMQP::Channel* ch = connection->readChannel();
	connection->withIoLock([&]() {
		ch->ack(tag);
	});
}

void RabbitMQClient::basicRejectImpl(Biterp::CallContext& ctx) {
	checkConnection();
	uint64_t tag = ctx.longParam();
	if (tag == 0) {
		throw Biterp::Error("Message tag cannot be empty!");
	}
	bool requeue = ctx.boolParamOptional(false);
	int flags = requeue ? AMQP::requeue : 0;
	AMQP::Channel* ch = connection->readChannel();
	connection->withIoLock([&]() {
		ch->reject(tag, flags);
	});
}

void RabbitMQClient::getQueueMessageCountImpl(Biterp::CallContext& ctx) {
	checkConnection();

	std::string name = ctx.stringParamUtf8();
	queueMessageCount = 0;
	AMQP::Channel* ch = connection->channel();
	connection->withIoLock([&]() {
		ch->declareQueue(name, AMQP::passive)
			.onSuccess([this](const std::string&, uint32_t count, uint32_t)
				{
					queueMessageCount = count;
					connection->loopbreak();
				})
			.onError([this](const char* message)
				{
					connection->loopbreak(message);
				});
	});
	connection->loop();
	ctx.setIntResult(queueMessageCount);
}

void RabbitMQClient::checkConnection() {
	if (!connection) {
		throw Biterp::Error("Connection is not established! Use the method Connect() first");
	}
	if (connection->isLost()) {
		throw Biterp::Error(consumerError.empty() ? "Connection lost" : consumerError);
	}
}

bool RabbitMQClient::connectionReady(std::string& errorOut) {
	if (!connection) {
		errorOut = "Connection is not established! Use the method Connect() first";
		return false;
	}
	if (connection->isLost()) {
		errorOut = consumerError.empty() ? "Connection lost" : consumerError;
		return false;
	}
	return true;
}

void RabbitMQClient::sleepNativeImpl(Biterp::CallContext& ctx) {

	uint64_t amount = ctx.longParam();
	std::this_thread::sleep_for(std::chrono::milliseconds(amount));
}

std::map<int, std::string> RabbitMQClient::propsFromJson(const json& object) {
	std::map<int, std::string> ret;
	int prop = CORRELATION_ID;
	for (const std::string& name : PROP_NAMES) {
		ret[prop++] = object.contains(name) ? object[name].get<std::string>() : "";
	}
	return ret;
}

AMQP::Table RabbitMQClient::headersFromJson(const std::string& propsJson, bool forConsume)
{
	if (propsJson.empty()) {
		return AMQP::Table();
	}
	json object;
	try {
		object = json::parse(propsJson);
	}
	catch (const json::parse_error& e) {
		throw Biterp::Error(std::string("JSON parse error: ") + e.what());
	}
	return headersFromJson(object, forConsume);
}

AMQP::Table RabbitMQClient::headersFromJson(const json& object, bool forConsume)
{
	AMQP::Table headers;
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
		else if (forConsume && name == "x-stream-offset")
		{
			headers.set(name, AMQP::Timestamp(Utils::parseDateTime(value)));
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

std::string RabbitMQClient::lastMessageHeaders() {
	AMQP::Table& headersTbl = lastMessage.headers;
	json hdr = json::object();
	for (const std::string& key : headersTbl.keys()) {
		hdr[key] = amqpFieldToJson(headersTbl.get(key));
	}
	return hdr.dump();
}
