
#include "RMQConnection.h"
#include "amqpcpp.h"
#include "MyTCPHandler.cpp"

bool RMQConnection::connect() {

	// create an instance of your own tcp handler
	MyTcpHandler myHandler;

	// address of the server
	AMQP::Address address("amqp://guest:guest@localhost/vhost");

	// create a AMQP connection object
	AMQP::TcpConnection connection(&myHandler, address);

	// and create a channel
	AMQP::TcpChannel channel(&connection);

	AMQP::ExchangeType topic = AMQP::ExchangeType::topic;

	// use the channel object to call the AMQP method you like
	channel.declareExchange("my-exchange", AMQP::fanout);
	channel.declareQueue("my-queue");
	channel.bindQueue("my-exchange", "my-queue", "my-routing-key");
}