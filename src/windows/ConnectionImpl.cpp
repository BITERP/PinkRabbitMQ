#include "ConnectionImpl.h"

ConnectionImpl::ConnectionImpl(const AMQP::Address& address, int timeout) : 
	handler(address.hostname(), address.port(), address.secure(), timeout),
	trChannel(nullptr)
{
	connection = new AMQP::Connection(&handler, address.login(), address.vhost());
	handler.setConnection(connection);
}

ConnectionImpl::~ConnectionImpl() {
	if (connection->usable()) {
		connection->close();
		handler.loopIteration();
	}
	handler.quitRead();
	handler.quit();
	delete connection;
}

void ConnectionImpl::connect() {
	const uint16_t timeout = 5000;
	std::chrono::milliseconds timeoutMs{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutMs;
	while (connection->waiting() && (end - std::chrono::system_clock::now()).count() > 0) {
		handler.loopIteration();
	}
	if (!connection->ready()) {
		throw Biterp::Error("Wrong login, password or vhost");
	}
	handler.setReceiveTimeout();
}


AMQP::Channel* ConnectionImpl::channel() {
	if (!trChannel || !trChannel->usable()) {
		if (trChannel) {
			trChannel->close();
			trChannel.reset(nullptr);
		}
		if (!connection->usable()) {
			throw Biterp::Error("Connection lost");
		}
		trChannel.reset(new AMQP::Channel(connection));
		trChannel->onReady([this]() {
				handler.quit();
			});
		trChannel->onError([this](const char* message) {
				LOGW("Channel closed with reason: " + string(message));
				trChannel = nullptr;
			});
		handler.loop();
	}
	if (!trChannel) {
		throw new Biterp::Error("Channel not opened");
	}
	return trChannel.get();
}

void ConnectionImpl::loop() {
	handler.loop();
}

void ConnectionImpl::loopbreak() {
	handler.quit();
}