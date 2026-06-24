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
	void loop();
	void loopbreak(std::string error = "");
	void shutdown();

	std::recursive_mutex& ioMutex() { return _ioMutex; }

	template<typename F>
	void withIoLock(F&& f) {
		std::lock_guard<std::recursive_mutex> lock(_ioMutex);
		f();
	}

private:
	ConnectionImpl* pimpl;
	int timeout;
	volatile bool broken;
	std::string error;
	std::mutex _mutex;
	std::condition_variable cvBroken;
	std::recursive_mutex _ioMutex;
};