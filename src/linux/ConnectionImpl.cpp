#include "ConnectionImpl.h"
#include "Connection.h"
#include <addin/biterp/Component.hpp>
#include <addin/biterp/Logger.hpp>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>

ConnectionImpl::ConnectionImpl(Connection& owner, const AMQP::Address& address, int connectTimeoutSec) :
    owner(owner),
    trChannel(nullptr), stop(false), connectTimeoutSec(connectTimeoutSec > 0 ? connectTimeoutSec : 5),
    shutDown(false)
{
    static bool sslInited = false;
    if (!sslInited){
        SSL_library_init();
        sslInited = true;
    }
    eventLoop = event_base_new();
    handler.reset(new TCPHandler(owner, eventLoop));
    connection.reset(new AMQP::TcpConnection(handler.get(), address));
    thread = std::thread(ConnectionImpl::loopThread, this);
}

ConnectionImpl::~ConnectionImpl() {
    shutdown();
    {
        std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
        connection.reset(nullptr);
        handler.reset(nullptr);
    }
    event_base_free(eventLoop);
}

void ConnectionImpl::shutdown() {
    if (shutDown) {
        return;
    }
    shutDown = true;
    closeChannel(rcChannel);
    closeChannel(trChannel);
    stop = true;
    if (connection && connection->usable()) {
        std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
        connection->close(true);
    }
    if (connection) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!connection->closed() && std::chrono::steady_clock::now() < deadline) {
            event_base_loop(eventLoop, EVLOOP_NONBLOCK);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    event_base_loopbreak(eventLoop);
    if (thread.joinable()) {
        thread.join();
    }
}

void ConnectionImpl::loopThread(ConnectionImpl* thiz) {
    event_base* loop = thiz->eventLoop;
    while(!thiz->stop) {
        try{
            thiz->handler->sendHeartbeatsIfNeeded();
            const int result = event_base_loop(loop, EVLOOP_NONBLOCK);
            if (result == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }catch(std::exception& ex){
            Biterp::Logging::error("Channel loop error: " + std::string(ex.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}


void ConnectionImpl::openChannel(std::unique_ptr<AMQP::TcpChannel>& channel) {
    if (channel) {
        releaseChannel(channel);
    }
    if (!connection->usable() || handler->isLost()) {
        throw Biterp::Error("Connection lost " + handler->getError());
    }
    std::mutex m;
    std::condition_variable cv;
    volatile bool ready = false;

    channel.reset(new AMQP::TcpChannel(connection.get()));
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

void ConnectionImpl::releaseChannel(std::unique_ptr<AMQP::TcpChannel>& channel) {
    channel.reset(nullptr);
}

void ConnectionImpl::closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel, std::string reason) {
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
    while (!connection->ready() && !connection->closed() && std::chrono::steady_clock::now() < end) {
        if (!handler->getError().empty()) {
            throw Biterp::Error(handler->getError());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!connection->ready()) {
        if (!handler->getError().empty()){
            throw Biterp::Error(handler->getError());
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
