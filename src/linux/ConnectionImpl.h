#pragma once

#include <amqpcpp.h>
#include <thread>
#include <memory>
#include "TCPHandler.h"

class ConnectionImpl{
public:
    ConnectionImpl(const AMQP::Address& address, int connectTimeoutSec);
    virtual ~ConnectionImpl();
    void connect();
    AMQP::Channel* channel();
    AMQP::Channel* readChannel();

private:
    void openChannel(std::unique_ptr<AMQP::TcpChannel>& channel);
    void closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel, std::string reason="");

    static void loopThread(ConnectionImpl* thiz);

private:
    event_base* eventLoop;
    std::unique_ptr<TCPHandler> handler;
    std::unique_ptr<AMQP::TcpConnection> connection;

    std::unique_ptr<AMQP::TcpChannel> trChannel;
    std::unique_ptr<AMQP::TcpChannel> rcChannel;
    std::thread thread;
    volatile bool stop;
    int connectTimeoutSec;
};
