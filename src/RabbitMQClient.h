#pragma once

#include "Connection.h"
#include <addin/biterp/Component.hpp>
#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>


using namespace std;

class RabbitMQClient : public Biterp::Component {
public:
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
public:
	RabbitMQClient() : Biterp::Component("RabbitMQClient"), priority(0), inConsume(false) {};

	virtual ~RabbitMQClient() { clear(); };

	inline bool connect(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::connectImpl, paParams, lSizeArray);
	}
	inline bool basicPublish(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::basicPublishImpl, paParams, lSizeArray);
	}
	inline bool basicCancel(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::basicCancelImpl, paParams, lSizeArray);
	}
	inline bool basicAck(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::basicAckImpl, paParams, lSizeArray);
	}
	inline bool basicReject(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::basicRejectImpl, paParams, lSizeArray);
	}
	inline bool deleteQueue(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::deleteQueueImpl, paParams, lSizeArray);
	}
	inline bool bindQueue(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::bindQueueImpl, paParams, lSizeArray);
	}
	inline bool unbindQueue(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::unbindQueueImpl, paParams, lSizeArray);
	}
	inline bool declareExchange(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::declareExchangeImpl, paParams, lSizeArray);
	}
	inline bool deleteExchange(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::deleteExchangeImpl, paParams, lSizeArray);
	}
	inline bool setPriority(tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::setPriorityImpl, paParams, lSizeArray);
	}

	inline bool basicConsume(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::basicConsumeImpl, paParams, lSizeArray, pvarRetValue);
	}
	inline bool basicConsumeMessage(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::basicConsumeMessageImpl, paParams, lSizeArray, pvarRetValue);
	}
	inline bool declareQueue(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::declareQueueImpl, paParams, lSizeArray, pvarRetValue);
	}
	inline bool getPriority(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::getPriorityImpl, paParams, lSizeArray, pvarRetValue);
	}
	inline bool getRoutingKey(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::getRoutingKeyImpl, paParams, lSizeArray, pvarRetValue);
	}
	inline bool getHeaders(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
		return wrapCall(this, &RabbitMQClient::getHeadersImpl, paParams, lSizeArray, pvarRetValue);
	}

	inline bool getMsgProp(tVariant* pvarPropVal, const long lPropNum) {
		return wrapLongCall(this, &RabbitMQClient::getMsgPropImpl, lPropNum, nullptr, 0, pvarPropVal);
	}
	inline bool setMsgProp(tVariant* varPropVal, const long lPropNum) {
		return wrapLongCall(this, &RabbitMQClient::setMsgPropImpl, lPropNum, varPropVal, 1);
	}

private:

	void connectImpl(Biterp::CallContext& ctx);

	void declareExchangeImpl(Biterp::CallContext& ctx);
	void deleteExchangeImpl(Biterp::CallContext& ctx);
	void declareQueueImpl(Biterp::CallContext& ctx);
	void deleteQueueImpl(Biterp::CallContext& ctx);
	void bindQueueImpl(Biterp::CallContext& ctx);
	void unbindQueueImpl(Biterp::CallContext& ctx);

	void basicPublishImpl(Biterp::CallContext& ctx);

	void basicConsumeImpl(Biterp::CallContext& ctx);
	void basicConsumeMessageImpl(Biterp::CallContext& ctx);
	void basicCancelImpl(Biterp::CallContext& ctx);
	void basicAckImpl(Biterp::CallContext& ctx);
	void basicRejectImpl(Biterp::CallContext& ctx);

	
	inline void getRoutingKeyImpl(Biterp::CallContext& ctx) { ctx.setStringResult(u16Converter.from_bytes(lastMessage.routingKey)); }
	inline void getHeadersImpl(Biterp::CallContext& ctx) { ctx.setStringResult(u16Converter.from_bytes(lastMessageHeaders())); }
	inline void setPriorityImpl(Biterp::CallContext& ctx) { priority = ctx.intParam(); }
	inline void getPriorityImpl(Biterp::CallContext& ctx) { ctx.setIntResult(lastMessage.priority); }

	inline void getMsgPropImpl(const long propNum, Biterp::CallContext& ctx) { ctx.setStringResult(u16Converter.from_bytes(lastMessage.msgProps[propNum])); }
	inline void setMsgPropImpl(const long propNum, Biterp::CallContext& ctx) { msgProps[propNum] = ctx.stringParamUtf8(); }

	AMQP::Table headersFromJson(const string& json);
	void checkConnection();
	string lastMessageHeaders();

	void clear();

private:
	struct MessageObject {
		string body;
		uint64_t messageTag = 0;
		int priority = 0;
		string routingKey;
		map<int, std::string> msgProps;
		AMQP::Table headers;
	};

private:
	map<int, string> msgProps;
	unique_ptr<Connection> connection;
	int priority;
	MessageObject lastMessage;
	vector<string> consumers;
	queue<MessageObject> messageQueue;
	mutex _mutex;
	volatile bool inConsume;
	condition_variable cvDataArrived;

private:

	template<typename T, typename Proc>
	bool wrapLongCall(T* obj, Proc proc, const long param, tVariant* paParams, const long lSizeArray,
		tVariant* pvarRetValue = nullptr) {
		bool result = false;
		try {
			skipAddError = false;
			lastError.clear();
			Biterp::CallContext ctx(memManager, paParams, lSizeArray, pvarRetValue);
			(obj->*proc)(param, ctx);
			result = true;
		}
		catch (std::exception& e) {
			string who = typeid(e).name();
			string what = e.what();
			LOGE(who + ": " + what);
			addError(what, who);
		}
		return result;
	}

};

