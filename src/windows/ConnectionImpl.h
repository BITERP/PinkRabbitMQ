#pragma once

#include <amqpcpp.h>
#include <thread>
#include "SimplePocoHandler.h"

class ConnectionImpl{
public:
	ConnectionImpl(const AMQP::Address& address, int timeout);
	virtual ~ConnectionImpl();
	void connect();
	AMQP::Channel* channel();
	void loop();
	void loopbreak(string error="");
	AMQP::Channel* readChannel();

private:
	void openChannel(unique_ptr<AMQP::Channel>& channel);
	void closeChannel(unique_ptr<AMQP::Channel>& channel);

private:
	int timeout;
	volatile bool broken;
	SimplePocoHandler handler;
	AMQP::Connection* connection;
	unique_ptr<AMQP::Channel> trChannel;
	unique_ptr<AMQP::Channel> rcChannel;
	std::thread thread;
	string error;
	mutex _mutex;
	condition_variable cvBroken;
};

