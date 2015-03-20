/*
Copyright (c) 2015, Bryan Neff
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <chrono>
#include <string>
#include <memory> //unique_ptr

#include "rocket/tcp_socket.h"

#include "rocket/tls_socket.h"
#include "rocket/tls_init.h"

#include "rocket/resolver.h"
#include "rocket/http_response.h"

int main(int argc, char *argv[])
{
    //std::string host = "www.google.com";
    //std::string port = "443";
    std::string host = "127.0.0.1";
    std::string port = "1234";

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

    rocket::tls_init init;
    std::unique_ptr<rocket::tls_socket> client( new rocket::tls_socket );
    client->hostname = "a.foo.com";
    
    for( auto result : results )
    {
        printf("using ip: %s\n", result.c_str());
        if( client->connect( result, std::strtoul(port.c_str(), NULL, 0), std::chrono::seconds(10) ) > 0 )
        {
            printf("connected to %s\n", result.c_str() );
            printf("INFO: %s\n", client->tls_info().c_str() );
            std::string request = "GET / HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
            client->send(request.c_str(), request.length(), std::chrono::seconds(10));

            rocket::http_response response;
            response.read_response( client.get(), std::chrono::seconds(10) );

            printf("%d %s %s\n", response.status_code, response.reason.c_str(), response.version.c_str() );

            for( auto header : response.headers )
            {
                printf("%s: %s\n", header.first.c_str(), header.second.c_str() );
            }

            //if( response.headers.find("content-length") != response.headers.end() )
            //    printf("CASE-INSENSITIVE FIND: %s\n", (response.headers.find("content-LENGTH"))->second.c_str() );

            std::string body = response.get_body();
            //printf("SIZE %d\n", body.length() );
            printf("%s\n", body.c_str());
        }
        else
        {
            printf("failed to connect\n");
        }

        //process 1 result
        break;
    }

    return 0;
}
