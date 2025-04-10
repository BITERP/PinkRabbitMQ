#pragma once

#include <amqpcpp/libevent.h>
#include <string>
#include <openssl/ssl.h>

class TCPHandler: public AMQP::LibEventHandler{
public:

    TCPHandler(struct event_base *evbase): AMQP::LibEventHandler(evbase) {}

    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
        error = message;
    }

    inline const std::string& getError(){
        return error;
    }

private:
    std::string error;
};