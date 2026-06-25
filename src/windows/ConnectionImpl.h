#pragma once

#include <amqpcpp.h>
#include <thread>
#include "SimplePocoHandler.h"

class Connection;

class ConnectionImpl{
public:
	ConnectionImpl(Connection& owner, const AMQP::Address& address, int connectTimeoutSec);
	virtual ~ConnectionImpl();
	void connect();
	void shutdown();
	AMQP::Channel* channel();
	AMQP::Channel* readChannel();
	void invalidateTransactionChannel();
	void invalidateReadChannel();

private:
	void openChannel(std::unique_ptr<AMQP::Channel>& channel);
	void releaseChannel(std::unique_ptr<AMQP::Channel>& channel);
	void closeChannel(std::unique_ptr<AMQP::Channel>& channel, std::string reason="");

private:
	Connection& owner;
	SimplePocoHandler handler;
	std::unique_ptr<AMQP::Connection> connection;
	std::unique_ptr<AMQP::Channel> trChannel;
	std::unique_ptr<AMQP::Channel> rcChannel;
	std::thread thread;
	int connectTimeoutSec;
	bool shutDown = false;
};
