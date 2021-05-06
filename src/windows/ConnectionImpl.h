#pragma once

#include <memory>
#include <amqpcpp.h>
#include "SimplePocoHandler.h"

class ConnectionImpl{
public:
	ConnectionImpl(const AMQP::Address& address, int timeout);
	virtual ~ConnectionImpl();
	AMQP::Channel* channel();
	void connect();
	void loop();
	void loopbreak();
private:
	SimplePocoHandler handler;
	AMQP::Connection* connection;
	unique_ptr<AMQP::Channel> trChannel;
};

