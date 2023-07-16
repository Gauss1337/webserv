#include "http_tcpServer_linux.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>

void log(const std::string &message)
{
    std::cout << message << std::endl;
}

TcpServer::TcpServer(std::string ip_address, int port) : m_ip_address(ip_address), m_port(port), m_socket(), m_new_socket(),
                                                         m_incomingMessage(),
                                                         m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)),
                                                         m_serverMessage(buildResponse())
{
    m_socketAddress.sin_family = AF_INET;
    m_socketAddress.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_ip_address.c_str(), &(m_socketAddress.sin_addr)) <= 0)
    {
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    if (startServer() != 0)
    {
        std::ostringstream ss;
        ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
        log(ss.str());
        throw std::runtime_error(ss.str().c_str());

    }
}


TcpServer::~TcpServer()
{
    closeServer();
}

int TcpServer::startServer()
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::runtime_error("setsockopt failed");
    }
    if (m_socket < 0)
    {
        throw std::runtime_error("Cannot create socket");
    }
    if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
    {
        throw std::runtime_error("Cannot connect socket to address");
    }
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags == -1) {
        std::ostringstream ss;
        ss << "Failed to get flags for server socket";
        throw std::runtime_error(ss.str());
    }
    flags |= O_NONBLOCK;
    if (fcntl(m_socket, F_SETFL, flags) == -1) {
        std::ostringstream ss;
        ss << "Failed to set server socket to non-blocking mode";
        throw std::runtime_error(ss.str());
    }
    return 0;
}

void TcpServer::closeServer()
{
    close(m_socket);
    close(m_new_socket);
    exit(0);
}

void TcpServer::startListen()
{
    if (listen(m_socket, 216) < 0)
    {
        throw std::runtime_error("Socket listen failed");
    }
    std::ostringstream ss;
    ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
    log(ss.str());

    fd_set master_set, read_set , write_set; 
    FD_ZERO(&master_set);
    FD_SET(m_socket, &master_set); 

    int max_fd = m_socket;

    while (true)
    {
        read_set = master_set; 
        write_set = master_set;
        if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
        {
            throw std::runtime_error("Failed to select from set");
        }
        for (int i = 0; i <= max_fd; ++i)
        {
            if (FD_ISSET(i, &read_set)) 
            {
                if (i == m_socket) 
                {
                    acceptConnection(m_new_socket);

                    FD_SET(m_new_socket, &master_set);
                    if (m_new_socket > max_fd)
                    {
                        max_fd = m_new_socket; 
                    }
                }
                else
                {
                    char buffer[BUFFER_SIZE] = {0};
                    int bytesReceived = read(i, buffer, BUFFER_SIZE);
                    if (bytesReceived < 0)
                    {
                        std::ostringstream ss;
                        ss << "Failed to read bytes from client connection " << i;
                        close(i);
                        FD_CLR(i, &master_set);
                        throw std::runtime_error(ss.str().c_str());
                    }
                    else if (bytesReceived == 0) 
                    {
                        close(i);
                        FD_CLR(i, &master_set);
                    }
                    else
                    {
                        std::ostringstream ss;
                        ss << "------ Received Request from client ------\n\n";
                        log(ss.str());

                        m_new_socket = i;
                        sendResponse();
                    }
                }
            }
        }
        // for (int i = 0; i <= max_fd; ++i)
        // {
        //     if (FD_ISSET(i, &write_set)) 
        //     {
        //     }
        // }
    }
}

void TcpServer::acceptConnection(int &new_socket)
{
    new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
    if (new_socket < 0)
    {
        std::ostringstream ss;
        ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
        throw std::runtime_error(ss.str().c_str());

    }

    int flags = fcntl(new_socket, F_GETFL, 0);
    if (flags == -1) {
        std::ostringstream ss;
        ss << "Failed to get flags for new socket";
        throw std::runtime_error(ss.str().c_str());

    }

    flags |= O_NONBLOCK;
    if (fcntl(new_socket, F_SETFL, flags) == -1) {
        std::ostringstream ss;
        ss << "Failed to set new socket to non-blocking mode";
        throw std::runtime_error(ss.str().c_str());

    }
}

std::string TcpServer::buildResponse()
{
    std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :) </p></body></html>";
    std::ostringstream ss;
    ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
        << htmlFile;

    return ss.str();
}

void TcpServer::sendResponse()
{
    long bytesSent;

    bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());

    if (bytesSent == m_serverMessage.size())
    {
        log("------ Server Response sent to client ------\n\n");
    }
    else
    {
        throw std::runtime_error("Error sending response to client");
    }
}