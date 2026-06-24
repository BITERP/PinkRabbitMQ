#include "Connection.h"
#include <addin/biterp/Error.hpp>
#include <chrono>

#if defined(__linux__)

#include <linux/ConnectionImpl.h>

#elif defined(_WIN32) || defined(_WIN64)

#include <windows/ConnectionImpl.h>

#else
#error "Unsupported platform"
#endif



Connection::Connection(const AMQP::Address& address, int timeout): timeout(timeout), broken(false), _lost(false) {
	pimpl = new ConnectionImpl(*this, address, timeout);
}

Connection::~Connection() {
	shutdown();
	delete pimpl;
	pimpl = nullptr;
}

void Connection::shutdown() {
	if (pimpl) {
		pimpl->shutdown();
	}
}

void Connection::connect() {
	_lost = false;
	error.clear();
	pimpl->connect();
}

void Connection::setLostCallback(std::function<void(const std::string&)> callback) {
	lostCallback = std::move(callback);
}

void Connection::notifyLost(std::string lostError) {
	if (lostError.empty()) {
		lostError = "Connection lost";
	}
	bool firstLost = false;
	std::function<void(const std::string&)> callback;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		firstLost = !_lost;
		_lost = true;
		callback = lostCallback;
	}
	loopbreak(lostError);
	if (firstLost && callback) {
		callback(lostError);
	}
}

AMQP::Channel* Connection::channel() {
	return pimpl->channel();
}

AMQP::Channel* Connection::readChannel() {
	return pimpl->readChannel();
}


void Connection::loop() {
	std::unique_lock<std::mutex> lock(_mutex);
	broken = false;
	error.clear();
	if (!cvBroken.wait_for(lock, std::chrono::seconds(timeout), [&] { return broken; })) {
		broken = false;
		//channel()->close();
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
