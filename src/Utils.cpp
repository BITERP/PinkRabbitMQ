#include "Utils.h"
#include <chrono>
#include <date/date.h>

time_t Utils::parseDateTime(const string& value) {
	stringstream ss(value);
	date::sys_seconds duration;
	date::from_stream(ss, "%FT%T%Ez", duration);
	if (ss.fail()) {
		throw std::runtime_error("Wrong date format: " + value);
	}
	return duration.time_since_epoch().count();
}

