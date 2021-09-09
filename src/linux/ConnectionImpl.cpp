#include "ConnectionImpl.h"
#include <addin/biterp/Component.hpp>
#include <mutex>
#include <condition_variable>

ConnectionImpl::ConnectionImpl(const AMQP::Address& address) : 
    trChannel(nullptr)
{
    static bool sslInited = false;
    if (!sslInited){
        SSL_library_init();
        sslInited = true;
    } 
    eventLoop = event_base_new();
    handler = new AMQP::LibEventHandler(eventLoop);
    connection = new AMQP::TcpConnection(handler, address);
    thread = std::thread(ConnectionImpl::loopThread, this);
}

ConnectionImpl::~ConnectionImpl() {
    closeChannel(trChannel);
    while (connection->usable()) {
        connection->close();
    }
    event_base_loopbreak(eventLoop);
    thread.join();
    delete connection;
    delete handler;

    event_base_free(eventLoop);
}

void ConnectionImpl::loopThread(ConnectionImpl* thiz) {
    event_base* loop = thiz->eventLoop;
    while(!thiz->connection->closed()) {
        event_base_loop(loop, EVLOOP_NONBLOCK);
    }
}


void ConnectionImpl::openChannel(std::unique_ptr<AMQP::TcpChannel>& channel) {
    if (channel) {
        closeChannel(channel);
    }
    if (!connection->usable()) {
        throw Biterp::Error("Connection lost");
    }
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    channel.reset(new AMQP::TcpChannel(connection));
    channel->onReady([&]() {
        std::unique_lock<std::mutex> lock(m);
        ready = true;
        cv.notify_all();
        });
    channel->onError([this, &channel](const char* message) {
        LOGW("Channel closed with reason: " + std::string(message));
        channel.reset(nullptr);
        });
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&] { return ready; });
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
    while (!connection->ready() &&  !connection->closed() && (end - std::chrono::system_clock::now()).count() > 0) {
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
