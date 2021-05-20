#include "Connection.h"
#include <thread>
#include <addin/biterp/Error.hpp>
#include <chrono>

#if defined(__linux__)

#include <linux/ConnectionImpl.h>

#elif defined(_WIN32) || defined(_WIN64)

#include <windows/ConnectionImpl.h>

#else
#error "Unsupported platform"
#endif



Connection::Connection(const AMQP::Address& address, int timeout): timeout(timeout), broken(false) {
	pimpl = new ConnectionImpl(address);
}

Connection::~Connection() {
	delete pimpl;
}

void Connection::connect() {
	pimpl->connect();
}

AMQP::Channel* Connection::channel() {
	return pimpl->channel();
}

AMQP::Channel* Connection::readChannel() {
	return pimpl->readChannel();
}


void Connection::loop() {
	std::unique_lock<std::mutex> lock(_mutex);
	if (!cvBroken.wait_for(lock, std::chrono::seconds(timeout), [&] { return broken; })) {
		broken = false;
		channel()->close();
		throw Biterp::Error("AMQP server timeout error");
	}
	broken = false;
	if (!error.empty()) {
		throw Biterp::Error(error);
	}
}

void Connection::loopbreak(std::string error) {
	std::unique_lock<std::mutex> lock(_mutex);
	this->error = error;
	broken = true;
	cvBroken.notify_all();
}
