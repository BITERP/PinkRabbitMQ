#pragma once

#include <amqpcpp/libevent.h>
#include <string>
#include <openssl/ssl.h>

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
