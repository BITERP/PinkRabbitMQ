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
    void loop();
	void quitRead();
	void resetQuitRead();
	void loopRead();
	static void loopThread(SimplePocoHandler* clazz);
    void quit();
	void loopIteration();

    bool connected() const;

private:

    SimplePocoHandler(const SimplePocoHandler&) = delete;
    SimplePocoHandler& operator=(const SimplePocoHandler&) = delete;

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
