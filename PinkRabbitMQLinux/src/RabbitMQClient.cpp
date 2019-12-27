
#include "RabbitMQClient.h"
#include <unistd.h>
#include <cassert>

/*ESTABLISHING CONNECTION*/

bool RabbitMQClient::connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost) {
	
    eventLoop = event_base_new();

    handler = new AMQP::LibEventHandler(eventLoop);
    
    connection = new AMQP::TcpConnection(handler, AMQP::Address(host, port, AMQP::Login(login, pwd), vhost));

    channel = openChannel();

    return checkChannel(channel);
}

/*PROCESSING EXCHANGES AND QUEUES*/

bool RabbitMQClient::declareExchange(const std::string& name, const std::string& type, bool onlyCheckIfExists, bool durable, bool autodelete) {

    lastError = "";

    if (!checkChannel(channel)) {
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
        lastError = "Exchange type not supported!";
        return false;
    }

    bool result = true;

    channel->declareExchange(name, topic, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0))
        .onSuccess([this]()
    {
        event_base_loopbreak(eventLoop);
    })
        .onError([&result, this](const char* message)
    {
        lastError = message;
        event_base_loopbreak(eventLoop);
        result = false;
    });

    event_base_dispatch(eventLoop);

    return result;
}

bool RabbitMQClient::deleteExchange(const std::string& name, bool ifunused) {

    lastError = "";
    bool result = true;

    if (!checkChannel(channel)) {
        return false;
    }

    channel->removeExchange(name, (ifunused ? AMQP::ifunused : 0))
        .onSuccess([this]()
    {
        event_base_loopbreak(eventLoop);
    })
        .onError([&result, this](const char* message)
    {
        lastError = message;
        event_base_loopbreak(eventLoop);
        result = false;
    });

    event_base_dispatch(eventLoop);

    return result;
}

std::string RabbitMQClient::declareQueue(const std::string& name, bool onlyCheckIfExists, bool durable, bool autodelete, uint16_t maxPriority) {

    lastError = "";
    bool result = true;

    if (!checkChannel(channel)) {
        return "";
    }

    AMQP::Table args = AMQP::Table();
    if (maxPriority != 0) {
        args.set("x-max-priority", maxPriority);
    }

    channel->declareQueue(name, (onlyCheckIfExists ? AMQP::passive : 0) | (durable ? AMQP::durable : 0) | (durable ? AMQP::durable : 0) | (autodelete ? AMQP::autodelete : 0), args)
        .onSuccess([this]()
    {
        event_base_loopbreak(eventLoop);
    })
        .onError([this](const char* message)
    {
        lastError = message;
        event_base_loopbreak(eventLoop);
    });

    event_base_dispatch(eventLoop);
    return name;
}

bool RabbitMQClient::deleteQueue(const std::string& name, bool ifunused, bool ifempty) {

    lastError = "";
    bool result = true;

    if (!checkChannel(channel)) {
        return "";
    }

    channel->removeQueue(name, (ifunused ? AMQP::ifunused : 0) | (ifempty ? AMQP::ifempty : 0))
        .onSuccess([this]()
    {
        event_base_loopbreak(eventLoop);
    })
        .onError([&result, this](const char* message)
    {
        lastError = message;
        event_base_loopbreak(eventLoop);
        result = false;
    });

    event_base_dispatch(eventLoop);

    return result;
}

bool RabbitMQClient::bindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

    lastError = "";
    bool result = true;

    if (!checkChannel(channel)) {
        return false;
    }

    channel->bindQueue(exchange, queue, routingKey)
        .onSuccess([this]()
    {
        event_base_loopbreak(eventLoop);
    })
        .onError([&result, this](const char* message)
    {
        lastError = message;
        event_base_loopbreak(eventLoop);
        result = false;
    });

    event_base_dispatch(eventLoop);

    return result;
}

bool RabbitMQClient::unbindQueue(const std::string& queue, const std::string& exchange, const std::string& routingKey) {

    lastError = "";
    bool result = true;

    if (!checkChannel(channel)) {
        return false;
    }

    channel->unbindQueue(exchange, queue, routingKey)
        .onSuccess([this]()
    {
        event_base_loopbreak(eventLoop);
    })
        .onError([&result, this](const char* message)
    {
        lastError = message; 
        event_base_loopbreak(eventLoop);
        result = false;
    });

    event_base_dispatch(eventLoop);

    return result;
}

/*SENDING MESSAGES*/

bool RabbitMQClient::basicPublish(std::string& exchange, std::string& routingKey, std::string& message, bool persistent) {

    lastError = "";
    bool result = true;

    if (!checkChannel(channel)) {
        return false;
    }

    AMQP::TcpChannel* channelLoc = openChannel();
    event_base_loopexit(eventLoop, NULL);
    channelLoc->onReady([this, &channelLoc, &message, &persistent, &exchange, &routingKey]() {
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
        channelLoc->publish(exchange, routingKey, envelope);
        event_base_loopbreak(eventLoop);
    });

    event_base_dispatch(eventLoop);

    channelLoc->close()
        .onSuccess([this]() {
        event_base_loopbreak(eventLoop);
    });

    event_base_dispatch(eventLoop);

    return result;
}

/*PROPERTIES*/


void RabbitMQClient::setMsgProp(int propNum, const std::string& val) {
    msgProps[propNum] = val;
}

std::string RabbitMQClient::getMsgProp(int propNum) {
    return msgProps[propNum];
}

bool RabbitMQClient::setPriority(int _priority) {
    priority = _priority;
    return true;
}

int RabbitMQClient::getPriority() {
    return priority;
}

/*RECEIVING MESSAGES*/

std::string RabbitMQClient::basicConsume(const std::string& queue, const int _selectSize) {

    lastError = "";

    if (!checkChannel(channel)) {
        return "";
    }

    channel->setQos(_selectSize, true);

    channel->consume(queue)
        .onMessage([this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
    {
        MessageObject* msgOb = new MessageObject();
        int leng = msgOb->body.length();
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
        lastError = message;
        event_base_loopbreak(eventLoop);
    });

    for (int i = 0; i < 1; i++) {
        threadPool.push(new std::thread(RabbitMQClient::loopThread, eventLoop));
    }

    return "";
}

void RabbitMQClient::loopThread(event_base* eventLoop) {
    event_base_loop(eventLoop, EVLOOP_NO_EXIT_ON_EMPTY);
    std::string t = "";
}

bool RabbitMQClient::basicConsumeMessage(std::string& outdata, std::uint64_t& outMessageTag, uint16_t timeout) {

    lastError = "";
    int size = readQueue->size();
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

    event_base_loopbreak(eventLoop);

    return false;
}

bool RabbitMQClient::basicAck(const std::uint64_t& messageTag) {

    lastError = "";

    if (!checkChannel(channel)) {
        return false;
    }

    if (messageTag == 0) {
        lastError = "Message tag cannot be empty!";
        return false;
    }

    channel->ack(messageTag);

    return true;
}

bool RabbitMQClient::basicReject(const std::uint64_t& messageTag) {

    lastError = "";

    if (!checkChannel(channel)) {
        return false;
    }

    if (messageTag == 0) {
        lastError = "Message tag cannot be empty!";
        return false;
    }

    channel->reject(messageTag);

    return true;
}

bool RabbitMQClient::basicCancel() {

    MessageObject* msgOb;
    ThreadSafeQueue<MessageObject*>::QueueResult result;
    readQueue->close();
    while ((result = readQueue->pop(msgOb)) != ThreadSafeQueue<MessageObject*>::CLOSED) {
        delete msgOb;
    };
        
    for (int i = 0; i < threadPool.size(); i++) {
        std::thread* curr = threadPool.front();
        curr->detach();
        threadPool.pop();
    }
    
    return true;
}

/*HELPERS*/

std::string RabbitMQClient::getLastError()
{
    return lastError;
}

bool RabbitMQClient::checkConnection() {
    lastError = "";
    if (connection == nullptr || !connection->ready()) {
        lastError = "Error. RabbitMQ connection is not open";
        return false;
    }
    return true;
}

bool RabbitMQClient::checkChannel(AMQP::TcpChannel* _channel) {

    if (_channel == nullptr || !_channel->usable()) {
        lastError = "Error. RabbitMQ channel is not in usable state";
        return false;
    }
    return true;
}

AMQP::TcpChannel* RabbitMQClient::openChannel() {

    AMQP::TcpChannel* channelLoc = new AMQP::TcpChannel(connection);

    channelLoc->onReady([this]() {
        std::cout << " Main channel is ready" << std::endl;
        event_base_loopbreak(eventLoop);
    });
    channelLoc->onError([this](const char* message) {
        lastError = message;
        event_base_loopbreak(eventLoop);
    });
    event_base_dispatch(eventLoop);

    return channelLoc;
}

void RabbitMQClient::updateLastError(const char* text) {
    lastError = text;
}

RabbitMQClient::~RabbitMQClient() {

    if (channel != nullptr) {
        channel->close();
        delete channel;
    }

    if (connection != nullptr) {
        connection->close();
    }

    while (!readQueue->empty()) {
        MessageObject* msgOb;
        readQueue->pop(msgOb);
        delete msgOb;
    }
    
    assert(readQueue->empty());

    event_base_free(eventLoop);
    libevent_global_shutdown();

    for (int i = 0; i < threadPool.size(); i++) {
        std::thread* curr = threadPool.front();
        curr->detach();
        threadPool.pop();
    }
   
}