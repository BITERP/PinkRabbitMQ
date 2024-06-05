#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cassert>
#include <iostream>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/RejectCertificateHandler.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/NetException.h>

#include "SimplePocoHandler.h"

using namespace Poco::Net;

namespace
{
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
		pollTimeout(0, 1)
	{
		initializeSSL();
		if (ssl) 
		{
			// Replace with AcceptCertificateHandler to skip cert verification 
			Poco::SharedPtr<InvalidCertificateHandler> pInvHandler = new AcceptCertificateHandler(false);
			Context::Ptr pContext = new Poco::Net::Context(Context::TLS_CLIENT_USE, "");
			SSLManager::instance().initializeClient(nullptr, pInvHandler, pContext);
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

	~SimplePocoHandlerImpl() {
		uninitializeSSL();
	}

	std::unique_ptr<Poco::Net::StreamSocket> socket;
	AMQP::Connection* connection;
	Buffer inputBuffer;
	Buffer outBuffer;
	std::vector<char> tmpBuff;
	Poco::Timespan pollTimeout;
};
SimplePocoHandler::SimplePocoHandler(const std::string& host, uint16_t port, bool ssl) :
	m_impl(new SimplePocoHandlerImpl(ssl, host))
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

void SimplePocoHandler::setConnection(AMQP::Connection* connection): stop(false)
{
	m_impl->connection = connection;
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
		}
		catch (const Poco::Net::ConnectionResetException& exc) {
			Biterp::Logging::error(exc.displayText());
			m_impl->connection->close();
		}
		catch (const Poco::Exception& exc)
		{
			std::string err = typeid(exc).name() + std::string(": ") + exc.displayText() + std::string(". ") + exc.what();
			Biterp::Logging::error(err);
			std::cerr << err << std::endl;
		}
	}
}

void SimplePocoHandler::loopIteration() {

	if (m_impl->socket->poll(m_impl->pollTimeout, 1)) {
		int avail = m_impl->connection->expected();
		if (!avail) { avail = 4; }
		while (avail > 0)
		{
			if (m_impl->tmpBuff.size() < avail)
			{
				m_impl->tmpBuff.resize(avail, 0);
			}
			int received = m_impl->socket->receiveBytes(&m_impl->tmpBuff[0], avail);
			if (received < 0) {
				break;
			}
			m_impl->inputBuffer.write(m_impl->tmpBuff.data(), received);
			avail = m_impl->socket->available();
		}
	}

	if (m_impl->socket->available() < 0)
	{
		std::cerr << "SOME socket error!!!" << std::endl;
	}

	if (m_impl->connection && m_impl->inputBuffer.available())
	{
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
}

void SimplePocoHandler::SimplePocoHandler::close()
{
	m_impl->socket->close();
}

void SimplePocoHandler::onData(
	AMQP::Connection* connection, const char* data, size_t size)
{
	m_impl->connection = connection;
	size_t written = m_impl->outBuffer.write(data, size);
	while (written != size)
	{
		sendDataFromBuffer();
		written += m_impl->outBuffer.write(data + written, size - written);
	}
}

void SimplePocoHandler::onReady(AMQP::Connection* connection)
{
}

void SimplePocoHandler::onError(
	AMQP::Connection* connection, const char* message)
{
	error = message;
	Biterp::Logging::error("AMQP error: " + error);
}

void SimplePocoHandler::onClosed(AMQP::Connection* connection)
{
}

uint16_t SimplePocoHandler::onNegotiate(AMQP::Connection* connection, uint16_t interval) {
	return interval;
}


void SimplePocoHandler::sendDataFromBuffer()
{
	if (m_impl->outBuffer.available())
	{
		m_impl->socket->sendBytes(m_impl->outBuffer.data(), m_impl->outBuffer.available());
		m_impl->outBuffer.drain();
	}
}

