#include <cstdio>
#include "RabbitMQClientTest.cpp"
#include <unistd.h>

int main()
{
    RabbitMQClientTest unit;
    unit.testDeclareSendReceive();
    sleep(2);
    return 0;
}

