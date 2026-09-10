#ifndef SRC_SIMPLEPOCOHANDLER_H_
#define SRC_SIMPLEPOCOHANDLER_H_

#include <memory>
#include <mutex>
#include <functional>
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
 	inline void stopLoop() {stop=true;}
	static void loopThread(SimplePocoHandler* obj) {obj->loopRead();};
	void loopIteration();
    inline const std::string& getError(){ return error;}
    inline bool isClosed(){ return closed;}

    void run(AMQP::Channel* channel, std::function<void(AMQP::Channel*)> proc);

private:

    SimplePocoHandler(const SimplePocoHandler&) = delete;
    SimplePocoHandler& operator=(const SimplePocoHandler&) = delete;

	void sendDataFromBuffer();
    void close();
    size_t parse(const char* data, size_t size);

    virtual void onData(AMQP::Connection *connection, const char *data, size_t size) override;

    virtual void onReady(AMQP::Connection *connection) override;

    virtual void onError(AMQP::Connection *connection, const char *message) override;

    virtual void onClosed(AMQP::Connection *connection) override;

    virtual uint16_t onNegotiate(AMQP::Connection* connection, uint16_t interval) override;

private:
    std::shared_ptr<SimplePocoHandlerImpl> m_impl;
    std::string error;
    volatile bool stop;
    bool closed;
    std::mutex run_mutex;
};

#endif /* SRC_SIMPLEPOCOHANDLER_H_ */
