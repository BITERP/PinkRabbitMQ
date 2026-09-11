#pragma once

#include <amqpcpp.h>
#include <thread>
#include <functional>
#include "SimplePocoHandler.h"

class ConnectionImpl{
public:
	ConnectionImpl(const AMQP::Address& address);
	virtual ~ConnectionImpl();
	void connect();
	AMQP::Channel* channel();
	AMQP::Channel* readChannel();
	inline void run(AMQP::Channel* channel, std::function<void(AMQP::Channel*)> proc) {handler.run(channel, proc);}

private:
	void openChannel(std::unique_ptr<AMQP::Channel>& channel);
	void closeChannel(std::unique_ptr<AMQP::Channel>& channel, std::string reason="");

private:
	SimplePocoHandler handler;
	std::unique_ptr<AMQP::Connection> connection;
	std::unique_ptr<AMQP::Channel> trChannel;
	std::unique_ptr<AMQP::Channel> rcChannel;
	std::thread thread;
};
