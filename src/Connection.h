#pragma once

#include <functional>
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
	void notifyLost(std::string error);
	bool isLost() const { return _lost; }
	void setLostCallback(std::function<void(const std::string&)> callback);
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
	volatile bool _lost;
	std::string error;
	std::function<void(const std::string&)> lostCallback;
	std::mutex _mutex;
	std::condition_variable cvBroken;
	std::recursive_mutex _ioMutex;
};