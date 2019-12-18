#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

class MyTcpHandler : public AMQP::TcpHandler
{

    /**
     *  Method that is called by the AMQP-CPP library when it wants to interact
     *  with the main event loop. The AMQP-CPP library is completely non-blocking,
     *  and only make "write()" or "read()" system calls when it knows in advance
     *  that these calls will not block. To register a filedescriptor in the
     *  event loop, it calls this "monitor()" method with a filedescriptor and
     *  flags telling whether the filedescriptor should be checked for readability
     *  or writability.
     *
     *  @param  connection      The connection that wants to interact with the event loop
     *  @param  fd              The filedescriptor that should be checked
     *  @param  flags           Bitwise or of AMQP::readable and/or AMQP::writable
     */
    virtual void monitor(AMQP::TcpConnection* connection, int fd, int flags) override
    {
        std::string test = "";
        // @todo
        //  add your own implementation, for example by adding the file
        //  descriptor to the main application event loop (like the select() or
        //  poll() loop). When the event loop reports that the descriptor becomes
        //  readable and/or writable, it is up to you to inform the AMQP-CPP
        //  library that the filedescriptor is active by calling the
        //  connection->process(fd, flags) method.
    }
};