#include "ConnectionImpl.h"
#include "Connection.h"
#include <chrono>

ConnectionImpl::ConnectionImpl(Connection& owner, const AMQP::Address& address, int connectTimeoutSec) :
	owner(owner),
	handler(address.hostname(), address.port(), address.secure()),
	trChannel(nullptr),
	connectTimeoutSec(connectTimeoutSec > 0 ? connectTimeoutSec : 5)
{
	handler.setIoMutex(&owner.ioMutex());
	handler.setLostCallback([&owner](const std::string& message) {
		owner.notifyLost(message);
	});
	connection.reset(new AMQP::Connection(&handler, address.login(), address.vhost()));
	handler.setConnection(connection.get());
	thread = std::thread(SimplePocoHandler::loopThread, &handler);
}

ConnectionImpl::~ConnectionImpl() {
	shutdown();
	handler.stopLoop();
	thread.join();
	{
		std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
		connection.reset(nullptr);
	}
}

void ConnectionImpl::shutdown() {
	closeChannel(rcChannel);
	closeChannel(trChannel);
	if (connection && connection->usable()) {
		std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
		connection->close();
	}
	if (connection) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (connection->usable() && std::chrono::steady_clock::now() < deadline) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}

void ConnectionImpl::openChannel(std::unique_ptr<AMQP::Channel>& channel) {
	if (channel) {
		closeChannel(channel);
	}
	if (!connection->usable() || handler.isClosed()) {
		throw Biterp::Error("Connection lost " + handler.getError());
	}
	std::mutex m;
	std::condition_variable cv;
	bool ready = false;
	channel.reset(new AMQP::Channel(connection.get()));
	channel->onReady([&]() {
		std::unique_lock<std::mutex> lock(m);
		ready = true;
		cv.notify_all();
		});
	channel->onError([&](const char* message) {
		closeChannel(channel, std::string(message));
		std::unique_lock<std::mutex> lock(m);
		ready = true;
		cv.notify_all();
		});
	std::unique_lock<std::mutex> lock(m);
	if (!cv.wait_for(lock, std::chrono::seconds(connectTimeoutSec), [&] { return ready; })) {
		channel.reset(nullptr);
		throw Biterp::Error("Channel open timeout");
	}
	if (!channel) {
		throw Biterp::Error("Channel not opened");
	}
	channel->onError([&](const char* message){closeChannel(channel, std::string(message));});
}

void ConnectionImpl::closeChannel(std::unique_ptr<AMQP::Channel>& channel, std::string reason) {
	if (!reason.empty()){
		Biterp::Logging::error("Channel closed with reason: " + reason);
	}
	if (channel && channel->usable()) {
		channel->close();
	}
	channel.reset(nullptr);
}


void ConnectionImpl::connect() {
	const uint16_t timeout = static_cast<uint16_t>(connectTimeoutSec * 1000);
	std::chrono::milliseconds timeoutMs{ timeout };
	auto end = std::chrono::system_clock::now() + timeoutMs;
	while (connection->waiting() && (end - std::chrono::system_clock::now()).count() > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	if (!connection->ready()) {
		if (!handler.getError().empty()){
			throw Biterp::Error(handler.getError());
		}
		throw Biterp::Error("Connection timeout.");
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
