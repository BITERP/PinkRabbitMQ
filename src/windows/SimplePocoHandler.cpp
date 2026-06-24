#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cassert>
#include <iostream>
#include <mutex>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/RejectCertificateHandler.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/NetException.h>

#include "SimplePocoHandler.h"
#include <mutex>

using namespace Poco::Net;

namespace
{
	void ensureSslInitialized()
	{
		static std::once_flag once;
		std::call_once(once, []() {
			Poco::Net::initializeSSL();
			Poco::SharedPtr<InvalidCertificateHandler> pInvHandler = new AcceptCertificateHandler(false);
			Context::Ptr pContext = new Poco::Net::Context(Context::TLS_CLIENT_USE, "");
			SSLManager::instance().initializeClient(nullptr, pInvHandler, pContext);
		});
	}

	class Buffer
	{
	public:
		explicit Buffer(size_t size) :
			m_data(size, 0),
			m_use(0)
		{
		}

		size_t write(const char* data, size_t size)
		{
			if (m_use == m_data.size())
			{
				return 0;
			}

			const size_t length = (size + m_use);
			size_t write = length < m_data.size() ? size : m_data.size() - m_use;
			memcpy(m_data.data() + m_use, data, write);
			m_use += write;
			return write;
		}

		void drain()
		{
			m_use = 0;
		}

		size_t available() const
		{
			return m_use;
		}

		const char* data() const
		{
			return m_data.data();
		}

		void shl(size_t count)
		{
			assert(count < m_use);

			const size_t diff = m_use - count;
			std::memmove(m_data.data(), m_data.data() + count, diff);
			m_use = m_use - count;
		}

	private:
		std::vector<char> m_data;
		size_t m_use;
	};
}

struct SimplePocoHandlerImpl
{
	SimplePocoHandlerImpl(bool ssl, const std::string& host) :
		connection(nullptr),
		inputBuffer(SimplePocoHandler::BUFFER_SIZE),
		outBuffer(SimplePocoHandler::BUFFER_SIZE),
		tmpBuff(SimplePocoHandler::TEMP_BUFFER_SIZE),
		pollTimeout(0, 100000)
	{
		if (ssl)
		{
			ensureSslInitialized();
			SecureStreamSocket* sslSocket = new SecureStreamSocket();
			sslSocket->setPeerHostName(host);
			sslSocket->setLazyHandshake(true);
			socket.reset(sslSocket);
		}
		else
		{
			socket.reset(new StreamSocket());
		}
	}

	~SimplePocoHandlerImpl() = default;

	std::unique_ptr<Poco::Net::StreamSocket> socket;
	AMQP::Connection* connection;
	Buffer inputBuffer;
	Buffer outBuffer;
	std::vector<char> tmpBuff;
	Poco::Timespan pollTimeout;
};
SimplePocoHandler::SimplePocoHandler(const std::string& host, uint16_t port, bool ssl) :
	m_impl(new SimplePocoHandlerImpl(ssl, host)), stop(false), closed(true)
{
	const Poco::Net::SocketAddress address(host, port);
	m_impl->socket->connect(address);
	m_impl->socket->setBlocking(true);
	m_impl->socket->setSendBufferSize(TEMP_BUFFER_SIZE);
	m_impl->socket->setReceiveBufferSize(TEMP_BUFFER_SIZE);
	m_impl->socket->setKeepAlive(true);
}

SimplePocoHandler::~SimplePocoHandler()
{
	close();
}

void SimplePocoHandler::setConnection(AMQP::Connection* connection)
{
	m_impl->connection = connection;
}

void SimplePocoHandler::setIoMutex(std::recursive_mutex* mutex)
{
	ioMutex = mutex;
}

void SimplePocoHandler::setLostCallback(std::function<void(const std::string&)> callback)
{
	lostCallback = std::move(callback);
}

void SimplePocoHandler::notifyLost(const std::string& message)
{
	if (!message.empty()) {
		error = message;
	}
	closed = true;
	if (lostCallback) {
		lostCallback(error.empty() ? "Connection lost" : error);
	}
}

void SimplePocoHandler::loopThread(SimplePocoHandler* obj)
{
	obj->loopRead();
}

void SimplePocoHandler::loopRead()
{
	while (!stop)
	{
		try
		{
			loopIteration();
			if (closed) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		catch (const Poco::Net::ConnectionResetException& exc) {
			Biterp::Logging::error(exc.displayText());
			if (m_impl->connection) {
				m_impl->connection->close();
			}
			notifyLost(exc.displayText());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		catch (const Poco::Net::NetException& exc) {
			Biterp::Logging::error(exc.displayText());
			notifyLost(exc.displayText());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		catch (const Poco::Exception& exc)
		{
			std::string err = typeid(exc).name() + std::string(": ") + exc.displayText() + std::string(". ") + exc.what();
			Biterp::Logging::error(err);
			std::cerr << err << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

void SimplePocoHandler::loopIteration() {
	bool receivedData = false;
	sendDataFromBuffer();
	sendHeartbeatsIfNeeded();

	if (m_impl->socket->poll(m_impl->pollTimeout, Poco::Net::Socket::SELECT_READ)) {
		int avail = m_impl->connection ? m_impl->connection->expected() : 0;
		if (!avail) { avail = 4; }
		while (avail > 0)
		{
			if (m_impl->tmpBuff.size() < static_cast<size_t>(avail))
			{
				m_impl->tmpBuff.resize(avail, 0);
			}
			int received = m_impl->socket->receiveBytes(&m_impl->tmpBuff[0], avail);
			if (received <= 0) {
				if (received == 0) {
					notifyLost("Connection closed by peer");
				}
				break;
			}
			receivedData = true;
			m_impl->inputBuffer.write(m_impl->tmpBuff.data(), received);
			avail = m_impl->socket->available();
		}
	}

	if (m_impl->socket->available() < 0)
	{
		notifyLost("Socket error");
	}

	if (m_impl->connection && m_impl->inputBuffer.available())
	{
		std::unique_lock<std::recursive_mutex> ioLock;
		if (ioMutex) {
			ioLock = std::unique_lock<std::recursive_mutex>(*ioMutex);
		}
		size_t count = m_impl->connection->parse(m_impl->inputBuffer.data(),
			m_impl->inputBuffer.available());

		if (count == m_impl->inputBuffer.available())
		{
			m_impl->inputBuffer.drain();
		}
		else if (count > 0) {
			m_impl->inputBuffer.shl(count);
		}
	}
	sendDataFromBuffer();
	if (!receivedData && !closed) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void SimplePocoHandler::close()
{
	m_impl->socket->close();
	closed = true;
}

void SimplePocoHandler::onData(
	AMQP::Connection* connection, const char* data, size_t size)
{
	m_impl->connection = connection;
	std::unique_lock<std::recursive_mutex> ioLock;
	if (ioMutex) {
		ioLock = std::unique_lock<std::recursive_mutex>(*ioMutex);
	}
	size_t written = m_impl->outBuffer.write(data, size);
	while (written != size)
	{
		sendDataFromBuffer();
		written += m_impl->outBuffer.write(data + written, size - written);
	}
}

void SimplePocoHandler::onReady(AMQP::Connection* connection)
{
	closed = false;
}

void SimplePocoHandler::onError(
	AMQP::Connection* connection, const char* message)
{
	error = message ? message : "";
	Biterp::Logging::error("AMQP error: " + error);
	notifyLost(error.empty() ? "AMQP connection error" : error);
}

void SimplePocoHandler::onClosed(AMQP::Connection* connection)
{
	notifyLost(error.empty() ? "Connection closed" : error);
}

uint16_t SimplePocoHandler::onNegotiate(AMQP::Connection* connection, uint16_t interval) {
	heartbeatInterval = interval;
	lastHeartbeatSent = std::chrono::steady_clock::now();
	return interval;
}

void SimplePocoHandler::onHeartbeat(AMQP::Connection* connection)
{
	lastHeartbeatSent = std::chrono::steady_clock::now();
}

void SimplePocoHandler::sendHeartbeatsIfNeeded()
{
	if (heartbeatInterval == 0 || !m_impl->connection || closed) {
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeatSent).count();
	if (elapsed >= heartbeatInterval / 2) {
		m_impl->connection->heartbeat();
		lastHeartbeatSent = now;
	}
}


void SimplePocoHandler::sendDataFromBuffer()
{
	if (m_impl->outBuffer.available())
	{
		m_impl->socket->sendBytes(m_impl->outBuffer.data(), m_impl->outBuffer.available());
		m_impl->outBuffer.drain();
	}
}
