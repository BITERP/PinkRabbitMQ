#pragma once

#include <string>
#include <amqpcpp.h>


class ConnectionImpl;

class Connection {
public:
	Connection(const AMQP::Address& address, int timeout);
	virtual ~Connection();
	void connect();
	AMQP::Channel* channel();
	void loop();
	void loopbreak(std::string error = "");
	AMQP::Channel* readChannel();

private:
	ConnectionImpl* pimpl;
};