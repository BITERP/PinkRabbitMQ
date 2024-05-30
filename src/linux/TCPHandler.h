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

    virtual bool onSecured(AMQP::TcpConnection *connection, const SSL *ssl) override
    {
        SSL_set_cipher_list((SSL*)ssl, "ALL:@SECLEVEL=0");
        return true;
    }


    inline const std::string& getError(){
        return error;
    }

private:
    std::string error;
};