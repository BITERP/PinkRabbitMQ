#pragma once

#include <amqpcpp.h>
#include <thread>
#include <memory>
#include "TCPHandler.h"

class ConnectionImpl{
public:
    ConnectionImpl(const AMQP::Address& address);
    virtual ~ConnectionImpl();
    void connect();
    AMQP::Channel* channel();
    AMQP::Channel* readChannel();

private:
    void openChannel(std::unique_ptr<AMQP::TcpChannel>& channel);
    void closeChannel(std::unique_ptr<AMQP::TcpChannel>& channel);

    static void loopThread(ConnectionImpl* thiz);

private:
    event_base* eventLoop;
    TCPHandler* handler;
    AMQP::TcpConnection* connection;

    std::unique_ptr<AMQP::TcpChannel> trChannel;
    std::thread thread;
};

