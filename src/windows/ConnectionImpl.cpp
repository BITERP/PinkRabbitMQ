#include "ConnectionImpl.h"

ConnectionImpl::ConnectionImpl(const AMQP::Address& address) : 
	handler(address.hostname(), address.port(), address.secure()),
	trChannel(nullptr)
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
	mutex m;
	condition_variable cv;
	bool ready = false;
	channel.reset(new AMQP::Channel(connection));
	channel->onReady([&]() {
		unique_lock<mutex> lock(m);
		ready = true;
		cv.notify_all();
		});
	channel->onError([this, &channel](const char* message) {
		LOGW("Channel closed with reason: " + string(message));
		channel.reset(nullptr);
		});
	unique_lock<mutex> lock(m);
	cv.wait(lock, [&] { return ready; });
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
	const uint16_t timeout = 10000;
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


AMQP::Channel* ConnectionImpl::readChannel() {
	if (!rcChannel || !rcChannel->usable()) {
		openChannel(rcChannel);
	}
	return rcChannel.get();
}
