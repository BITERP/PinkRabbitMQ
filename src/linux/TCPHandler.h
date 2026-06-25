#pragma once

#include <amqpcpp/flags.h>
#include <amqpcpp/linux_tcp.h>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <openssl/ssl.h>
#include <event2/event.h>
#include "Connection.h"

/**
 * Linux AMQP handler: libevent I/O with ioMutex around TcpConnection::process()
 * so parse (background thread) and publish/consume (application thread) do not race.
 * Fixes "frame size exceeded" when consuming and publishing on the same connection (#51).
 */
class TCPHandler: public AMQP::TcpHandler {
public:
	TCPHandler(Connection& owner, struct event_base *evbase)
		: _evbase(evbase), owner(owner), lost(true), tcpConnection(nullptr), heartbeatInterval(0) {}

	void onError(AMQP::TcpConnection *connection, const char *message) override
	{
		(void)connection;
		error = message ? message : "";
		lost = true;
		owner.notifyLost(error.empty() ? "Connection lost" : error);
	}

	void onConnected(AMQP::TcpConnection *connection) override
	{
		lost = false;
		error.clear();
		tcpConnection = connection;
	}

	void onLost(AMQP::TcpConnection *connection) override
	{
		(void)connection;
		lost = true;
		tcpConnection = nullptr;
		if (error.empty()) {
			error = "Connection lost";
		}
		owner.notifyLost(error);
	}

	uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override
	{
		(void)connection;
		heartbeatInterval = interval;
		lastHeartbeatSent = std::chrono::steady_clock::now();
		return interval;
	}

	void onHeartbeat(AMQP::TcpConnection *connection) override
	{
		(void)connection;
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

	void monitor(AMQP::TcpConnection *connection, int fd, int flags) override
	{
		auto iter = _watchers.find(fd);
		if (iter == _watchers.end()) {
			if (flags == 0) {
				return;
			}
			_watchers[fd] = std::unique_ptr<Watcher>(new Watcher(_evbase, connection, &owner, fd, flags));
		}
		else if (flags == 0) {
			_watchers.erase(iter);
		}
		else {
			iter->second->events(flags);
		}
	}

	const std::string& getError() const {
		return error;
	}

	bool isLost() const {
		return lost;
	}

private:
	class Watcher {
	private:
		struct CallbackContext {
			AMQP::TcpConnection* connection = nullptr;
			Connection* owner = nullptr;
		};

		static void callback(evutil_socket_t fd, short what, void *contextArg)
		{
			auto* ctx = static_cast<CallbackContext*>(contextArg);
			int amqpFlags = 0;
			if (what & EV_READ) {
				amqpFlags |= AMQP::readable;
			}
			if (what & EV_WRITE) {
				amqpFlags |= AMQP::writable;
			}
			std::lock_guard<std::recursive_mutex> lock(ctx->owner->ioMutex());
			ctx->connection->process(fd, amqpFlags);
		}

	public:
		Watcher(struct event_base *evbase, AMQP::TcpConnection *connection, Connection *owner, int fd, int events)
			: _context{connection, owner}
		{
			short eventFlags = EV_PERSIST;
			if (events & AMQP::readable) {
				eventFlags |= EV_READ;
			}
			if (events & AMQP::writable) {
				eventFlags |= EV_WRITE;
			}
			_event = event_new(evbase, fd, eventFlags, callback, &_context);
			event_add(_event, nullptr);
		}

		~Watcher()
		{
			event_del(_event);
			event_free(_event);
		}

		void events(int events)
		{
			event_del(_event);
			short eventFlags = EV_PERSIST;
			if (events & AMQP::readable) {
				eventFlags |= EV_READ;
			}
			if (events & AMQP::writable) {
				eventFlags |= EV_WRITE;
			}
			event_assign(_event, event_get_base(_event), event_get_fd(_event), eventFlags,
				event_get_callback(_event), event_get_callback_arg(_event));
			event_add(_event, nullptr);
		}

	private:
		struct event* _event;
		CallbackContext _context;
	};

	struct event_base* _evbase;
	std::map<int, std::unique_ptr<Watcher>> _watchers;
	Connection& owner;
	AMQP::TcpConnection* tcpConnection;
	std::string error;
	bool lost;
	uint16_t heartbeatInterval;
	std::chrono::steady_clock::time_point lastHeartbeatSent;
};
