
#include "RabbitMQClient.h"

bool RabbitMQClient::connect(const std::string& host, const uint16_t port, const std::string& login, const std::string& pwd, const std::string& vhost) {
	
    // access to the event loop
    eventLoop = event_base_new();

    // handler for libevent
    AMQP::LibEventHandler handler(eventLoop);
    
    AMQP::Address address(host, port, AMQP::Login(login, pwd), vhost);
    // make a connection
    connection = new AMQP::TcpConnection(&handler, address);

    // we need a channel too
    channel = new AMQP::TcpChannel(connection);

    // create a temporary queue
    channel->declareQueue("my_Tests5").onSuccess([this](const std::string& name, uint32_t messagecount, uint32_t consumercount) {

        // report the name of the temporary queue
        std::cout << "declared queue " << name << std::endl;
        
        // now we can close the connection
       // connection.close();
        event_base_loopexit(eventLoop, NULL);
        
    });

    event_base_dispatch(eventLoop);

    return true;
}

RabbitMQClient::~RabbitMQClient() {
    event_base_free(eventLoop);

    if (channel != nullptr) {
        channel->close();
        delete channel;
    }

    if (connection != nullptr) {
        connection->close();
    }
}