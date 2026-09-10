#pragma once

#include <amqpcpp/libevent.h>
#include <string>
#include <functional>
#include <mutex>
#include <openssl/ssl.h>

class TCPConnection: public AMQP::TcpConnection{

    public:
        using AMQP::TcpConnection::TcpConnection;

        virtual size_t onReceived(AMQP::TcpState *state, const AMQP::Buffer &buffer) override
        {
            std::lock_guard<std::mutex> lock(run_mutex);
            return AMQP::TcpConnection::onReceived(state, buffer);
        }

        void run(AMQP::Channel* channel, std::function<void(AMQP::Channel*)> proc){
            std::lock_guard<std::mutex> lock(run_mutex);
            proc(channel);
        }

    private:
        std::mutex run_mutex;

};

class TCPHandler: public AMQP::LibEventHandler{
public:

    TCPHandler(struct event_base *evbase): AMQP::LibEventHandler(evbase), lost(true) {}

    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
        error = message;
    }

    virtual void onConnected(AMQP::TcpConnection *connection)
    {
        lost = false;
    }

    virtual void onLost(AMQP::TcpConnection *connection) override
    {
        lost = true;
    }

    inline const std::string& getError(){
        return error;
    }

    inline const bool isLost(){
        return lost;
    }

private:
    std::string error;
    bool lost;
};
