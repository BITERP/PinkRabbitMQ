#include <addin/test/LinuxCTest.hpp>
#include <addin/test/Connection.hpp>
#include <chrono>

using namespace Biterp::Test;

CTEST(Sleep) {
	Connection con;
	tVariant paParams[1];
	con.intParam(&paParams[0], 1000);
	con.raiseErrors = true;
	auto end = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);
	ASSERT(con.callAsProc(u"Sleep", paParams, 1));
	
	ASSERT(std::chrono::system_clock::now() >= end);
}


CTEST_RUN();
