#pragma once

#include <amqpcpp/libevent.h>
#include <chrono>
#include <string>
#include <openssl/ssl.h>
#include "Connection.h"

class TCPHandler: public AMQP::LibEventHandler{
public:

    TCPHandler(Connection& owner, struct event_base *evbase)
        : AMQP::LibEventHandler(evbase), owner(owner), lost(true), tcpConnection(nullptr), heartbeatInterval(0) {}

    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
        error = message ? message : "";
        lost = true;
        owner.notifyLost(error.empty() ? "Connection lost" : error);
    }

    virtual void onConnected(AMQP::TcpConnection *connection) override
    {
        lost = false;
        error.clear();
        tcpConnection = connection;
    }

    virtual void onLost(AMQP::TcpConnection *connection) override
    {
        lost = true;
        tcpConnection = nullptr;
        if (error.empty()) {
            error = "Connection lost";
        }
        owner.notifyLost(error);
    }

    virtual uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override
    {
        heartbeatInterval = interval;
        lastHeartbeatSent = std::chrono::steady_clock::now();
        return interval;
    }

    virtual void onHeartbeat(AMQP::TcpConnection *connection) override
    {
        lastHeartbeatSent = std::chrono::steady_clock::now();
    }

    void sendHeartbeatsIfNeeded()
    {
        if (heartbeatInterval == 0 || !tcpConnection || lost) {
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeatSent).count();
        if (elapsed < heartbeatInterval / 2) {
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(owner.ioMutex());
        if (tcpConnection && !lost) {
            tcpConnection->heartbeat();
            lastHeartbeatSent = now;
        }
    }

    inline const std::string& getError(){
        return error;
    }

    inline const bool isLost(){
        return lost;
    }

private:
    Connection& owner;
    AMQP::TcpConnection* tcpConnection;
    std::string error;
    bool lost;
    uint16_t heartbeatInterval;
    std::chrono::steady_clock::time_point lastHeartbeatSent;
};
