#include <stdio.h>
#include <chrono>
#include <string>
#include <memory> //shared_ptr

#include "rocket/tcp_socket.h"

#include "rocket/tls_socket.h"

#include "rocket/resolver.h"
#include "rocket/http_response.h"

int main(int argc, char *argv[])
{
    //std::string host = "yams-dev.multisight.com";
    std::string host = "www.google.com";
    std::string port = "443";

    std::string error_string;
    auto results = rocket::resolver::resolve(host, port, AF_UNSPEC, error_string);
    if( results.size() == 0 )
    {
        printf("rocket::resolver: %s\n", error_string.c_str() );
        return -1;
    }
    
    /*
    for( auto result : results )
    {
        printf("ip: %s\n", result.c_str());
    }
    */

    //std::shared_ptr<rocket::tcp_socket> client( new rocket::tcp_socket );
    rocket::tls_init init;
    std::shared_ptr<rocket::tls_socket> client( new rocket::tls_socket );
    client->hostname = host;
    
    for( auto result : results )
    {
        printf("using ip: %s\n", result.c_str());
        //if( client->connect( results[0], std::strtoul(port.c_str(), NULL, 0), std::chrono::seconds(10) ) )
        if( client->connect( result, std::strtoul(port.c_str(), NULL, 0), std::chrono::seconds(10) ) > 0 )
        {
            printf("connected to %s\n", result.c_str() );
            std::string request = "GET / HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
            client->send(request.c_str(), request.length(), std::chrono::seconds(10));

            rocket::http_response response;
            response.read_response( client, std::chrono::seconds(10) );

            for( auto header : response.headers )
            {
                printf("%s: %s\n", header.first.c_str(), header.second.c_str() );
            }

            if( response.headers.find("content-length") != response.headers.end() )
                printf("CASE-INSENSITIVE FIND: %s\n", (response.headers.find("content-LENGTH"))->second.c_str() );

            //std::string body = response.get_body();
            //printf("SIZE %d\n", body.length() );
            //printf("%s\n", body.c_str());
        }
        else
        {
            printf("failed to connect\n");
        }
    }

    return 0;
}
