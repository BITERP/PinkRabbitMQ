#include "ConnectionImpl.h"
#include <addin/biterp/Component.hpp>
#include <addin/biterp/Logger.hpp>
#include <mutex>
#include <condition_variable>

ConnectionImpl::ConnectionImpl(const AMQP::Address& address) : 
    trChannel(nullptr), stop(false)
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
    closeChannel(trChannel);
    stop = true;
    event_base_loopbreak(eventLoop);
    thread.join();
    connection.reset(nullptr);
    handler.reset(nullptr);

    event_base_free(eventLoop);
}

void ConnectionImpl::loopThread(ConnectionImpl* thiz) {
    event_base* loop = thiz->eventLoop;
    while(!thiz->stop) {
        try{
            event_base_loop(loop, EVLOOP_NONBLOCK);
        }catch(std::exception& ex){
            Biterp::Logging::error("Channel loop error: " + std::string(ex.what()));
        }
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

    channel.reset(new AMQP::TcpChannel(connection.get()));
    channel->onReady([&]() {
        std::unique_lock<std::mutex> lock(m);
        ready = true;
        cv.notify_all();
        });
    channel->onError([&](const char* message) {
        Biterp::Logging::error("Channel closed with reason: " + std::string(message));
        channel.reset(nullptr);
        std::unique_lock<std::mutex> lock(m);
        ready = true;
        cv.notify_all();
        });
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&] { return ready; });
    if (!channel) {
        throw Biterp::Error("Channel not opened");
    }
}

void ConnectionImpl::closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel) {
    //if (channel && channel->usable()) {
    //    channel->close();
    //}
    channel.reset(nullptr);
}


void ConnectionImpl::connect() {    
    const uint16_t timeout = 15000;
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
    return channel();
}
