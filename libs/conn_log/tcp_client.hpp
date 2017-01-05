//=============================================================================
//
// Name:        tcp_client.hpp
// Authors:     James H. Loving
// Description: This file declares the tcp_client class, used for basic socket
//              communication.
//
//              Note: Modified from http://www.binarytides.com/
//              code-a-simple-socket-client-class-in-c/
//
//=============================================================================

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
    Socket-based network communication over IPv4/TCP.
*/
class tcp_client
{
    private:
        int sock;                   /**< network socket */
        std::string address;        /**< IPv4 destination address */
        int port;                   /**< TCP destination port */
        struct sockaddr_in server;  /**< connected server */
         
    public:
        /**
            Initialize the TCP client.
        */
        tcp_client();

        /**
            Connect to a server over IPv4/TCP.

            \param address IPv4 address of server.
            \param port TCP port on server.

            \return True to indicate connection success. Throws error on fail.
        */
        bool conn(std::string, int);

        /**
            Send string-encoded data to the server.

            \param data Data to send.

            \return True to indicate transmission success. Throws error on fail.
        */
        bool send_data(std::string data);

        /**
            Receive string-encoded data from the server.

            \param size Number of characters to receive.

            \return String-encoded reply.
        */
        std::string receive(int);
};
