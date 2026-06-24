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
    trChannel(nullptr), stop(false), connectTimeoutSec(connectTimeoutSec > 0 ? connectTimeoutSec : 5)
{
    static bool sslInited = false;
    if (!sslInited){
        SSL_library_init();
        sslInited = true;
    }
    eventLoop = event_base_new();
    handler.reset(new TCPHandler(eventLoop));
    connection.reset(new AMQP::TcpConnection(handler.get(), address));
    thread = std::thread(ConnectionImpl::loopThread, this);
}

ConnectionImpl::~ConnectionImpl() {
    shutdown();
    stop = true;
    event_base_loopbreak(eventLoop);
    thread.join();
    {
        std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
        connection.reset(nullptr);
        handler.reset(nullptr);
    }
    event_base_free(eventLoop);
}

void ConnectionImpl::shutdown() {
    closeChannel(rcChannel);
    closeChannel(trChannel);
    if (connection && connection->usable()) {
        std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
        connection->close();
    }
}

void ConnectionImpl::loopThread(ConnectionImpl* thiz) {
    event_base* loop = thiz->eventLoop;
    while(!thiz->stop) {
        try{
            int result = 0;
            {
                std::unique_lock<std::recursive_mutex> lock(thiz->owner.ioMutex(), std::try_to_lock);
                if (lock.owns_lock()) {
                    result = event_base_loop(loop, EVLOOP_NONBLOCK);
                }
            }
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
        closeChannel(channel);
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
        closeChannel(channel, message);
        std::unique_lock<std::mutex> lock(m);
        ready = true;
        cv.notify_all();
        });
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&] { return ready; });
    if (!channel) {
        throw Biterp::Error("Channel not opened");
    }
    channel->onError([&](const char* message){closeChannel(channel, message);});
}

void ConnectionImpl::closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel, std::string reason) {
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
    while (!connection->ready() && !connection->closed() && (end - std::chrono::system_clock::now()).count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!connection->ready()) {
        if (!handler->getError().empty()){
            throw Biterp::Error(handler->getError());
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
