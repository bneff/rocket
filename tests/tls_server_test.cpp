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

#include "rocket/tls_socket.h"
#include "rocket/tls_init.h"
#include "rocket/resolver.h"

#include <string.h>

#include <memory>

int main(int argc, char *argv[])
{
    rocket::tls_init init;
    init.set_private_key( "private.key" );
    init.set_public_key( "public.crt" );

    rocket::tls_socket socket;

    socket.bind("127.0.0.1", 1234);  //Set port to 0 for ephemeral
    auto my_addr = socket.get_local_address();
    printf("Socket bound to %s %d\n", my_addr.first.c_str(), my_addr.second );
    socket.listen();
    while( true )
    {
        std::unique_ptr<rocket::tcp_socket> client = socket.accept();
        auto peer = client->get_peer_address();
        printf("Accepted connection from %s : %d\n", peer.first.c_str(), peer.second );
        char tmp[2048];
        memset(tmp, 0, sizeof(tmp));
        ssize_t bytes_received = client->recv(tmp, sizeof(tmp), std::chrono::seconds(10));
        if( bytes_received > 0 )
        {
            printf("received %d bytes\n", bytes_received);
            std::string body = "<html><body>Hello World</body></html>";
            std::string neff("HTTP/1.1 200 OK\r\nHeader1: temptemptempcontinuation\r\nConnection: Close\r\nContent-Length: " + std::to_string( body.length() ) + "\r\n\r\n" + body);
            client->send(neff.c_str(), neff.length(), std::chrono::seconds(10));

            bytes_received = client->recv(tmp, sizeof(tmp), std::chrono::seconds(5));
            if( bytes_received == 0 )
                client->close();
        }
    }
    return 0;
}
