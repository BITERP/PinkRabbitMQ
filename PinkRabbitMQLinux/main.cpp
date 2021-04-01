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
    unit.testPassEmptyParameters();
    unit.testSendMessage();
    unit.testDeclareSendReceive();
    unit.testSSL();
    unit.testPublishFail();
    unit.testHeaders();
    sleep(2);
    return 0;
}

