#pragma once
#include "event2/event.h"
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>

class RabbitMQClient{
public:
	RabbitMQClient(const RabbitMQClient&) = delete;
	RabbitMQClient(const RabbitMQClient&&) = delete;
	RabbitMQClient& operator = (const RabbitMQClient&) = delete;
	RabbitMQClient& operator = (const RabbitMQClient&&) = delete;
	RabbitMQClient() = default;
	~RabbitMQClient();

	bool connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost);

private:
	event_base* eventLoop;
	AMQP::TcpConnection* connection;
	AMQP::TcpChannel* channel;
};