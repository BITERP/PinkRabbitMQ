#include "ConnectionImpl.h"

ConnectionImpl::ConnectionImpl(const AMQP::Address& address, int timeout) : 
	handler(address.hostname(), address.port(), address.secure()),
	trChannel(nullptr),
	timeout(timeout),
	broken(false)
{
	connection = new AMQP::Connection(&handler, address.login(), address.vhost());
	handler.setConnection(connection);
	thread = std::thread(SimplePocoHandler::loopThread, &handler);
}

ConnectionImpl::~ConnectionImpl() {
	closeChannel(trChannel);
	if (connection->usable()) {
		connection->close();
	}
	thread.join();
	delete connection;
}

void ConnectionImpl::openChannel(unique_ptr<AMQP::Channel>& channel) {
	if (channel) {
		closeChannel(channel);
	}
	if (!connection->usable()) {
		throw Biterp::Error("Connection lost");
	}
	channel.reset(new AMQP::Channel(connection));
	channel->onReady([this]() {
		loopbreak();
		});
	channel->onError([this, &channel](const char* message) {
		LOGW("Channel closed with reason: " + string(message));
		channel.reset(nullptr);
		loopbreak(message);
		});
	loop();
	if (!channel) {
		throw Biterp::Error("Channel not opened");
	}
}

void ConnectionImpl::closeChannel(unique_ptr<AMQP::Channel>& channel) {
	if (channel && channel->usable()) {
		channel->close();
	}
	channel.reset(nullptr);
}


void ConnectionImpl::connect() {
	const uint16_t timeout = 5000;
	std::chrono::milliseconds timeoutMs{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutMs;
	while (connection->waiting() && (end - std::chrono::system_clock::now()).count() > 0) {
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

void ConnectionImpl::loop() {
	unique_lock<mutex> lock(_mutex);
	if (!cvBroken.wait_for(lock, chrono::seconds(timeout), [&] { return broken; })) {
		broken = false;
		channel()->close();
		throw Biterp::Error("AMQP server timeout error");
	}
	broken = false;
	if (!error.empty()) {
		throw Biterp::Error(error);
	}
}

void ConnectionImpl::loopbreak(string error) {
	unique_lock<mutex> lock(_mutex);
	this->error = error;
	broken = true;
	cvBroken.notify_all();
}

AMQP::Channel* ConnectionImpl::readChannel() {
	return channel();
}
