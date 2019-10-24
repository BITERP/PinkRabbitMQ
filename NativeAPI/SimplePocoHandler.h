#ifndef SRC_SIMPLEPOCOHANDLER_H_
#define SRC_SIMPLEPOCOHANDLER_H_

#include <memory>
#include <amqpcpp.h>
#include "RabbitMQClient.h"

struct SimplePocoHandlerImpl;
class SimplePocoHandler: public AMQP::ConnectionHandler
{
public:

    static constexpr size_t BUFFER_SIZE = 80 * 1024 * 1024; //8Mb
    static constexpr size_t TEMP_BUFFER_SIZE = 10 * 1024 * 1024; //1Mb

    SimplePocoHandler(const std::string& host, uint16_t port);
    virtual ~SimplePocoHandler();

    void loop();
	void quitRead();
	void resetQuitRead();
	void loop(uint16_t timeout);
	static void loopThread(SimplePocoHandler* clazz, uint16_t timeout);
    void quit();

    bool connected() const;

private:

    SimplePocoHandler(const SimplePocoHandler&) = delete;
    SimplePocoHandler& operator=(const SimplePocoHandler&) = delete;

	void loopIteration();
	void sendDataFromBuffer();
    void close();

    virtual void onData(
            AMQP::Connection *connection, const char *data, size_t size);

    virtual void onConnected(AMQP::Connection *connection);

    virtual void onError(AMQP::Connection *connection, const char *message);

    virtual void onClosed(AMQP::Connection *connection);

    std::shared_ptr<SimplePocoHandlerImpl> m_impl;
};

#endif /* SRC_SIMPLEPOCOHANDLER_H_ */
