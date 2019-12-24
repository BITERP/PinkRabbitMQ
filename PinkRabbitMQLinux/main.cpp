#include <cstdio>
#include "RabbitMQClientTest.cpp"
#include <unistd.h>

int main()
{
    RabbitMQClientTest unit;
    unit.testSendReceive();
    sleep(2);
    return 0;
}

