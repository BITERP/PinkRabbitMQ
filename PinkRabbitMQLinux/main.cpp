#include <cstdio>
#include "src/RabbitMQClient.h"

int main()
{
    RabbitMQClient client;
    client.connect("devdevopsrmq.bit-erp.loc", 5672, "rkudakov_devops", "rkudakov_devops", "rkudakov_devops");
    printf("hello from PinkRabbitMQLinux!\n");
    return 0;
}