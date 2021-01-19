#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cassert>
#include <iostream>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/RejectCertificateHandler.h>
#include <Poco/Net/SSLManager.h>

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
		connected(false),
		connection(nullptr),
		quit(false),
		quitRead(false),
		inputBuffer(SimplePocoHandler::BUFFER_SIZE),
		outBuffer(SimplePocoHandler::BUFFER_SIZE),
		tmpBuff(SimplePocoHandler::TEMP_BUFFER_SIZE)
	{
		initializeSSL();
		if (ssl) 
		{
			// Replace with AcceptCertificateHandler to skip cert verification 
			Poco::SharedPtr<InvalidCertificateHandler> pCert = new RejectCertificateHandler(false);
			Context::Ptr pContext = new Poco::Net::Context(Context::TLSV1_2_CLIENT_USE, "");
			SSLManager::instance().initializeClient(0, pCert, pContext);
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
	bool connected;
	AMQP::Connection* connection;
	bool quit;
	bool quitRead;
	Buffer inputBuffer;
	Buffer outBuffer;
	std::vector<char> tmpBuff;
};
SimplePocoHandler::SimplePocoHandler(const std::string& host, uint16_t port, bool ssl) :
	m_impl(new SimplePocoHandlerImpl(ssl, host))
{
	const Poco::Net::SocketAddress address(host, port);
	m_impl->socket->connect(address);
	m_impl->socket->setBlocking(true);
	m_impl->socket->setReceiveTimeout(Poco::Timespan(5, 0));
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

void SimplePocoHandler::loopThread(SimplePocoHandler* clazz)
{
	clazz->resetQuitRead();
	clazz->loopRead();
}

void SimplePocoHandler::loopRead()
{

	try
	{
		while (!m_impl->quitRead)
		{
			loopIteration();
		}
	}
	catch (const Poco::Exception& exc)
	{
		std::cerr << "Poco exception " << exc.displayText();
	}
}

void SimplePocoHandler::loop()
{
	try
	{
		while (!m_impl->quit)
		{
			loopIteration();
		}

		if (m_impl->quit && m_impl->outBuffer.available())
		{
			sendDataFromBuffer();
		}

	}
	catch (const Poco::Exception& exc)
	{
		std::cerr << "Poco exception " << exc.displayText();
	}
	m_impl->quit = false; // reset channel for repeatable using
}

void SimplePocoHandler::loopIteration() {

	sendDataFromBuffer();

	int avail = m_impl->connection->expected();
	while (avail > 0)
	{
		if (m_impl->tmpBuff.size() < avail)
		{
			m_impl->tmpBuff.resize(avail, 0);
		}

		int recieved = m_impl->socket->receiveBytes(&m_impl->tmpBuff[0], avail);
		if (recieved <= 0) {
			std::cerr << "received " << recieved << " of " << avail << std::endl;
		}
		if (recieved < 0) {
			break;
		}
		m_impl->inputBuffer.write(m_impl->tmpBuff.data(), recieved);
		avail = m_impl->socket->available();
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

void SimplePocoHandler::quit()
{
	m_impl->quit = true;
}

void SimplePocoHandler::quitRead()
{
	m_impl->quitRead = true;
}

void SimplePocoHandler::resetQuitRead()
{
	m_impl->quitRead = false;
}

void SimplePocoHandler::SimplePocoHandler::close()
{
	m_impl->socket->close();
}

void SimplePocoHandler::onData(
	AMQP::Connection* connection, const char* data, size_t size)
{
	m_impl->connection = connection;
	const size_t writen = m_impl->outBuffer.write(data, size);
	if (writen != size)
	{
		sendDataFromBuffer();
		m_impl->outBuffer.write(data + writen, size - writen);
	}
}

void SimplePocoHandler::onConnected(AMQP::Connection* connection)
{
	m_impl->connected = true;
}

void SimplePocoHandler::onError(
	AMQP::Connection* connection, const char* message)
{
	std::cerr << "AMQP error " << message << std::endl;
}

void SimplePocoHandler::onClosed(AMQP::Connection* connection)
{
	std::cout << "AMQP closed connection" << std::endl;
	m_impl->quit = true;
}

bool SimplePocoHandler::connected() const
{
	return m_impl->connected;
}

void SimplePocoHandler::sendDataFromBuffer()
{
	if (m_impl->outBuffer.available())
	{
		m_impl->socket->sendBytes(m_impl->outBuffer.data(), m_impl->outBuffer.available());
		m_impl->outBuffer.drain();
	}
}

