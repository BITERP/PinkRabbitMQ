#include <cstdio>
#include "RabbitMQClientTest.cpp"
#include <unistd.h>

int main(int argc, char** argv)
{
    std::string configFile;
    if (argc > 1) {
        configFile = argv[1];
    }
    RabbitMQClientTest unit(configFile);
    unit.testDefParams();
    unit.testPublishFail();
    //unit.testPassEmptyParameters();
    //unit.testSendMessage();
    //unit.testDeclareSendReceive();
    unit.testSSL();
    sleep(2);
    return 0;
}

