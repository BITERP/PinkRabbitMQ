#include "ConnectionImpl.h"
#include "Connection.h"
#include <chrono>

ConnectionImpl::ConnectionImpl(Connection& owner, const AMQP::Address& address, int connectTimeoutSec) :
	owner(owner),
	handler(address.hostname(), address.port(), address.secure()),
	trChannel(nullptr),
	connectTimeoutSec(connectTimeoutSec > 0 ? connectTimeoutSec : 5),
	shutDown(false)
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
	{
		std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
		connection.reset(nullptr);
	}
}

void ConnectionImpl::shutdown() {
	if (shutDown) {
		return;
	}
	shutDown = true;
	closeChannel(rcChannel);
	closeChannel(trChannel);
	handler.stopLoop();
	handler.closeSocket();
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
	if (thread.joinable()) {
		thread.join();
	}
}

void ConnectionImpl::openChannel(std::unique_ptr<AMQP::Channel>& channel) {
	if (channel) {
		releaseChannel(channel);
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
		if (message && message[0]) {
			Biterp::Logging::error("Channel open error: " + std::string(message));
		}
		std::unique_lock<std::mutex> lock(m);
		ready = true;
		cv.notify_all();
		});
	std::unique_lock<std::mutex> lock(m);
	if (!cv.wait_for(lock, std::chrono::seconds(connectTimeoutSec), [&] { return ready; })) {
		releaseChannel(channel);
		throw Biterp::Error("Channel open timeout");
	}
	if (!channel || !channel->usable()) {
		releaseChannel(channel);
		throw Biterp::Error("Channel not opened");
	}
	channel->onError([&](const char* message) {
		if (message && message[0]) {
			Biterp::Logging::error("Channel error: " + std::string(message));
		}
	});
}

void ConnectionImpl::releaseChannel(std::unique_ptr<AMQP::Channel>& channel) {
	channel.reset(nullptr);
}

void ConnectionImpl::closeChannel(std::unique_ptr<AMQP::Channel>& channel, std::string reason) {
	if (!reason.empty()){
		Biterp::Logging::error("Channel closed with reason: " + reason);
	}
	releaseChannel(channel);
}

void ConnectionImpl::invalidateTransactionChannel() {
	releaseChannel(trChannel);
}

void ConnectionImpl::invalidateReadChannel() {
	releaseChannel(rcChannel);
}


void ConnectionImpl::connect() {
	const uint16_t timeout = static_cast<uint16_t>(connectTimeoutSec * 1000);
	std::chrono::milliseconds timeoutMs{ timeout };
	auto end = std::chrono::steady_clock::now() + timeoutMs;
	while (connection->waiting() && std::chrono::steady_clock::now() < end) {
		if (!handler.getError().empty()) {
			throw Biterp::Error(handler.getError());
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	if (!connection->ready()) {
		if (!handler.getError().empty()){
			throw Biterp::Error(handler.getError());
		}
		throw Biterp::Error("Connection timeout.");
	}
}


AMQP::Channel* ConnectionImpl::channel() {
	if (trChannel && !trChannel->usable()) {
		releaseChannel(trChannel);
	}
	if (!trChannel) {
		openChannel(trChannel);
	}
	return trChannel.get();
}


AMQP::Channel* ConnectionImpl::readChannel() {
	if (rcChannel && !rcChannel->usable()) {
		releaseChannel(rcChannel);
	}
	if (!rcChannel) {
		openChannel(rcChannel);
	}
	return rcChannel.get();
}
