
#include "RMQConnection.h"
#include "event2/event.h"
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include "MyTCPHandler.cpp"

bool RMQConnection::connect() {
	
    // access to the event loop
    auto evbase = event_base_new();

    // handler for libevent
    AMQP::LibEventHandler handler(evbase);
    
    // make a connection
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://rkudakov_devops:rkudakov_devops@devdevopsrmq.bit-erp.loc/rkudakov_devops"));

    // we need a channel too
    AMQP::TcpChannel channel(&connection);

    // create a temporary queue
    channel.declareQueue("my_Tests4").onSuccess([&connection](const std::string& name, uint32_t messagecount, uint32_t consumercount) {

        // report the name of the temporary queue
        std::cout << "declared queue " << name << std::endl;

        // now we can close the connection
        connection.close();
    });

    // run the loop
    event_base_dispatch(evbase);

    event_base_free(evbase);

    // done
    
    return 0;
}