#include "Connection.h"

#if defined(__linux__)

#include <linux/ConnectionImpl.h>

#elif defined(_WIN32) || defined(_WIN64)

#include <windows/ConnectionImpl.h>

#else
#error "Unsupported platform"
#endif

Connection::Connection(const AMQP::Address& address, int timeout) {
	pimpl = new ConnectionImpl(address, timeout);
}

Connection::~Connection() {
	delete pimpl;
}

AMQP::Channel* Connection::channel() {
	return pimpl->channel();
}

void Connection::connect() {
	pimpl->connect();
}

void Connection::loop() {
	pimpl->loop();
}

void Connection::loopbreak() {
	pimpl->loopbreak();
}
