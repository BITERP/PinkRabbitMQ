#ifndef SRC_SIMPLEPOCOHANDLER_H_
#define SRC_SIMPLEPOCOHANDLER_H_

#include <memory>
#include <amqpcpp.h>
#include "RabbitMQClient.h"

struct SimplePocoHandlerImpl;
class SimplePocoHandler: public AMQP::ConnectionHandler
{
public:

    static constexpr size_t BUFFER_SIZE = 8 * 1024 * 1024; //8Mb
    static constexpr size_t TEMP_BUFFER_SIZE = 1 * 1024 * 1024; //1Mb

    SimplePocoHandler(const std::string& host, uint16_t port, bool ssl);
    virtual ~SimplePocoHandler();

    void setConnection(AMQP::Connection* connection);
 	void loopRead();
	static void loopThread(SimplePocoHandler* clazz);
	void loopIteration();

private:

    SimplePocoHandler(const SimplePocoHandler&) = delete;
    SimplePocoHandler& operator=(const SimplePocoHandler&) = delete;

	void sendDataFromBuffer();
    void close();

    virtual void onData(
            AMQP::Connection *connection, const char *data, size_t size) override;

    virtual void onReady(AMQP::Connection *connection) override;

    virtual void onError(AMQP::Connection *connection, const char *message) override;

    virtual void onClosed(AMQP::Connection *connection) override;

    virtual uint16_t onNegotiate(AMQP::Connection* connection, uint16_t interval) override;

    std::shared_ptr<SimplePocoHandlerImpl> m_impl;
    std::string error;

};

#endif /* SRC_SIMPLEPOCOHANDLER_H_ */
