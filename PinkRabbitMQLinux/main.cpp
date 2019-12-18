#include <cstdio>
#include "src/RMQConnection.h"

int main()
{
    RMQConnection conn;
    conn.connect();
    printf("hello from PinkRabbitMQLinux!\n");
    return 0;
}