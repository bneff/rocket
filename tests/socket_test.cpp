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
#include <string>
#include <chrono>

#include "rocket/tcp_socket.h"
#include "rocket/resolver.h"

#include <string.h>

int main(int argc, char *argv[])
{
    /*
    auto results = rocket::resolver::resolve("www.google.com", "80", AF_UNSPEC);
    for( auto result : results )
    {
        printf("ip: %s\n", result.c_str());
    }

    rocket::tcp_socket client;
    
    if( client.connect( results[0], 80, std::chrono::seconds(10) ) )
        printf("connected\n");
    else
        printf("not connected\n");
    */

    rocket::tcp_socket socket;
    socket.bind("::1", 1234);  //Set port to 0 for ephemeral
    auto my_addr = socket.get_local_address();
    printf("Socket bound to %s %d\n", my_addr.first.c_str(), my_addr.second );
    socket.listen();
    while( true )
    {
        rocket::tcp_socket client = socket.accept();
        auto peer = client.get_peer_address();
        printf("Accepted connection from %s : %d\n", peer.first.c_str(), peer.second );
        char tmp[2048];
        memset(tmp, 0, sizeof(tmp));
        ssize_t bytes_received = client.recv(tmp, sizeof(tmp), std::chrono::seconds(10));
        while( bytes_received > 0 )
        {
            printf("received %d bytes %s\n", bytes_received, &tmp);
            std::string neff("HTTP/1.1 200 OK\r\nHeader1: temptemptemp\r\n  ,continuation\r\nTransfer-Encoding: chunked\r\n\r\n5\r\ntest1\r\n5\r\ntest2\r\n0\r\n");
            client.send(neff.c_str(), neff.length(), std::chrono::seconds(10));

            memset(tmp, 0, sizeof(tmp));
            bytes_received = client.recv(tmp, sizeof(tmp), std::chrono::seconds(10));
        }
    }
    return 0;
}
