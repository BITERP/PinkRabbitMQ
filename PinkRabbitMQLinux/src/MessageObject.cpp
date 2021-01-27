#include <string>
#include <map>

struct MessageObject {
	std::string body;
	uint64_t messageTag;
	int priority;
	std::string routingKey;
	std::map<int, std::string> msgProps;
};