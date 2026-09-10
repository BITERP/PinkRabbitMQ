#pragma once

#include <string>
#include <mutex>
#include <condition_variable>
#include <amqpcpp.h>

class ConnectionImpl;

class Connection {
public:
	Connection(const AMQP::Address& address, int timeout);
	virtual ~Connection();
	void connect();
	AMQP::Channel* channel();
	AMQP::Channel* readChannel();
	void loopbreak(std::string error = "");
	inline void run(std::function<void(AMQP::Channel*)> proc) {runOnChannel(channel(), proc);}
	void runOnChannel(AMQP::Channel* channel, std::function<void(AMQP::Channel*)> proc);

private:
	void loop();

private:
	ConnectionImpl* pimpl;
	int timeout;
	volatile bool broken;
	std::string error;
	std::mutex _mutex;
	std::condition_variable cvBroken;
};
