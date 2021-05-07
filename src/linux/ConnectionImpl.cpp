#include "ConnectionImpl.h"
#include <addin/biterp/Component.hpp>

ConnectionImpl::ConnectionImpl(const AMQP::Address& address) : 
	trChannel(nullptr)
{
	eventLoop = event_base_new();
    handler = new AMQP::LibEventHandler(eventLoop);
    connection = new AMQP::TcpConnection(handler, address);
//	thread = std::thread(SimplePocoHandler::loopThread, &handler);
}

ConnectionImpl::~ConnectionImpl() {
	closeChannel(trChannel);
	if (connection->usable()) {
		connection->close();
	}
	//thread.join();
	delete connection;
	delete handler;

    event_base_free(eventLoop);
    libevent_global_shutdown();
}

void ConnectionImpl::openChannel(std::unique_ptr<AMQP::TcpChannel>& channel) {
	if (channel) {
		closeChannel(channel);
	}
	if (!connection->usable()) {
		throw Biterp::Error("Connection lost");
	}
	channel.reset(new AMQP::TcpChannel(connection));
	channel->onReady([&]() {
		event_base_loopbreak(eventLoop);
		});
	channel->onError([this, &channel](const char* message) {
		LOGW("Channel closed with reason: " + std::string(message));
		channel.reset(nullptr);
		});
	event_base_dispatch(eventLoop);
	if (!channel) {
		throw Biterp::Error("Channel not opened");
	}
}

void ConnectionImpl::closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel) {
	if (channel && channel->usable()) {
		channel->close();
	}
	channel.reset(nullptr);
}


void ConnectionImpl::connect() {	
	const uint16_t timeout = 5000;
	std::chrono::milliseconds timeoutMs{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutMs;
	while (!connection->ready() && (end - std::chrono::system_clock::now()).count() > 0) {
		this_thread::sleep_for(chrono::milliseconds(100));
	}
	if (!connection->ready()) {
		throw Biterp::Error("Wrong login, password or vhost");
	}
}


AMQP::Channel* ConnectionImpl::channel() {
	if (!trChannel || !trChannel->usable()) {
		openChannel(trChannel);
	}
	return trChannel.get();
}


AMQP::Channel* ConnectionImpl::readChannel() {
	return channel();
}
