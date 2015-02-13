#include <stdio.h>
#include <chrono>
#include <string>
#include <memory> //shared_ptr

#include "rocket/tcp_socket.h"
#include "rocket/resolver.h"
#include "rocket/http_response.h"

int main(int argc, char *argv[])
{
    //auto results = rocket::resolver::resolve("www.google.com", "80", AF_UNSPEC);
    //auto results = rocket::resolver::resolve("172.30.42.1", "80", AF_UNSPEC);
    auto results = rocket::resolver::resolve("192.168.1.1", "80", AF_UNSPEC);
    //auto results = rocket::resolver::resolve("www.pelco.com", "80", AF_UNSPEC);
    for( auto result : results )
    {
        printf("ip: %s\n", result.c_str());
    }

    std::shared_ptr<rocket::tcp_socket> client( new rocket::tcp_socket );
    
    if( client->connect( results[0], 80, std::chrono::seconds(10) ) )
    {
        printf("connected to %s\n", results[0].c_str() );
        std::string request = "GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n";
        client->send(request.c_str(), request.length(), std::chrono::seconds(10));

        rocket::http_response response;
        response.read_response( client, std::chrono::seconds(10) );
    }
    else
    {
        printf("failed to connect\n");
    }

    return 0;
}
