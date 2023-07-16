#include "http_tcpServer_linux.hpp"

#include <iostream>
#include <stdexcept>
#include "http_tcpServer_linux.hpp"

int main()
{
    try
    {
        TcpServer server("127.0.0.1", 8080);
        server.startListen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}


