#include <stdio.h>
#include <chrono>
#include <string>
#include <memory> //shared_ptr

#include "rocket/tcp_socket.h"
#include "rocket/resolver.h"
#include "rocket/http_response.h"

int main(int argc, char *argv[])
{
    std::string host = "www.pelco.com";
    //std::string host = "www.google.com";
    //std::string host = "172.30.42.1";
    std::string port = "80";
    //std::string host = "::1";
    //std::string port = "1234";


    std::string error_string;
    auto results = rocket::resolver::resolve(host, port, AF_UNSPEC, error_string);
    if( results.size() == 0 )
    {
        printf("rocket::resolver: %s\n", error_string.c_str() );
        return -1;
    }
    
    for( auto result : results )
    {
        printf("ip: %s\n", result.c_str());
    }

    std::shared_ptr<rocket::tcp_socket> client( new rocket::tcp_socket );
    
    if( client->connect( results[0], std::strtoul(port.c_str(), NULL, 0), std::chrono::seconds(10) ) )
    {
        printf("connected to %s\n", results[0].c_str() );
        std::string request = "GET / HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
        client->send(request.c_str(), request.length(), std::chrono::seconds(10));

        rocket::http_response response;
        response.read_response( client, std::chrono::seconds(10) );

        for( auto header : response.headers )
        {
            printf("%s: %s\n", header.first.c_str(), header.second.c_str() );
        }

        printf("CASE-INSENSITIVE FIND: %s\n", (response.headers.find("content-LENGTH"))->second.c_str() );

        std::string body = response.get_body();
        printf("SIZE %d\n", body.length() );
        printf("%s\n", body.c_str());
    }
    else
    {
        printf("failed to connect\n");
    }

    return 0;
}
