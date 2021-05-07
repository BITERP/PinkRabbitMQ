#pragma once

#include <amqpcpp.h>
#include <thread>
#include <amqpcpp/libevent.h>
#include <memory>

class ConnectionImpl{
public:
	ConnectionImpl(const AMQP::Address& address);
	virtual ~ConnectionImpl();
	void connect();
	AMQP::Channel* channel();
	AMQP::Channel* readChannel();

private:
	void openChannel(std::unique_ptr<AMQP::TcpChannel>& channel);
	void closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel);

private:
	event_base* eventLoop;
	AMQP::LibEventHandler* handler;
	AMQP::TcpConnection* connection;

	std::unique_ptr<AMQP::TcpChannel> trChannel;
	std::thread thread;
};

