/*
    tcp_client.cpp
    James H. Loving
    modified from http://www.binarytides.com/code-a-simple-socket-client-class-in-c/
*/

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

class tcp_client
{
private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;
     
public:
    tcp_client();
    bool conn(std::string, int);
    bool send_data(std::string data);
    std::string receive(int);
};
 
tcp_client::tcp_client()
{
    sock = -1;
    address = "";
    port = 0;
}
 
// Connect to a host on a certain port number
bool tcp_client::conn(std::string address , int port)
{
    //create socket if it is not already created
    if (sock == -1)
    {
        //Create socket
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
            throw std::runtime_error("Could not create socket");
        }
         
        std::cout << "Socket created\n";
    }
     
    //setup address structure
    if(inet_addr(address.c_str()) == -1)
    {
        //resolve the hostname, since it's not an ip address

        struct hostent *he;
        struct in_addr **addr_list;
         
        if ((he = gethostbyname(address.c_str())) == NULL)
        {
            throw std::runtime_error("Gethostbyname failed");
            return false;
        }
         
        addr_list = (struct in_addr **) he->h_addr_list;
 
        for(int i = 0; addr_list[i] != NULL; i++)
        {
            //strcpy(ip , inet_ntoa(*addr_list[i]) );
            server.sin_addr = *addr_list[i];
             
            std::cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<std::endl;
             
            break;
        }
    }
    else
    {
        // address is a plain IP address, so directly assign
        server.sin_addr.s_addr = inet_addr(address.c_str());
    }
     
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
     
    //Connect to remote server
    if (connect(sock, (struct sockaddr*) &server, sizeof(server)) < 0)
    {
        throw std::runtime_error("Socket connection failed");
    }
     
    std::cout << "Connected\n";

    return true;
}
 
// Send data to the connected host
bool tcp_client::send_data(std::string data)
{
    if(send(sock, data.c_str(), strlen(data.c_str()), 0) < 0)
    {
        throw std::runtime_error("Socket send failed");
    }
     
    return true;
}
 
// Receive data from the connected host
std::string tcp_client::receive(int size=512)
{
    char buffer[size];
    std::string reply;
     
    if( recv(sock , buffer , sizeof(buffer) , 0) < 0)
    {
        puts("recv failed");
    }
     
    reply = buffer;
    return reply;
}
