#include <string>
#include <map>
#include <amqpcpp.h>

struct MessageObject {
	std::string body;
	uint64_t messageTag;
	int priority;
	std::string routingKey;
	std::map<int, std::string> msgProps;
	AMQP::Table headers;
};