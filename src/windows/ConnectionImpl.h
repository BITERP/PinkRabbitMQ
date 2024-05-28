#pragma once

#include <amqpcpp.h>
#include <thread>
#include "SimplePocoHandler.h"

class ConnectionImpl{
public:
	ConnectionImpl(const AMQP::Address& address);
	virtual ~ConnectionImpl();
	void connect();
	AMQP::Channel* channel();
	AMQP::Channel* readChannel();

private:
	void openChannel(std::unique_ptr<AMQP::Channel>& channel);
	void closeChannel(std::unique_ptr<AMQP::Channel>& channel);

private:
	SimplePocoHandler handler;
	AMQP::Connection* connection;
	std::unique_ptr<AMQP::Channel> trChannel;
	std::unique_ptr<AMQP::Channel> rcChannel;
	std::thread thread;
};

