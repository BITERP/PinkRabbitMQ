#include <string>
#include <map>

struct MessageObject {
	std::string body;
	uint64_t messageTag;
	std::map<int, std::string> msgProps;
};