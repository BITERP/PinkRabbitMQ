#pragma once

#include <memory>
#include <amqpcpp.h>


class ConnectionImpl;

class Connection {
public:
	Connection(const AMQP::Address& address, int timeout);
	virtual ~Connection();
	AMQP::Channel* channel();
	void connect();
	void loop();
	void loopbreak();

private:
	ConnectionImpl* pimpl;
};