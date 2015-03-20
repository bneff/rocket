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
#ifndef rocket_tcp_socket
#define rocket_tcp_socket

//SOMAXCONN
#include <sys/socket.h>
#include <memory>

namespace rocket
{


class tcp_socket
{
    friend class tls_socket;
public:

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

    tcp_socket(tcp_socket&& other);
    tcp_socket& operator=(tcp_socket&& other);

    tcp_socket();
    virtual ~tcp_socket();

    virtual ssize_t connect( std::string host, uint16_t port, std::chrono::milliseconds millis );
    virtual ssize_t close();
    virtual ssize_t bind( std::string host, uint16_t port );
    virtual ssize_t listen( size_t max_connections=SOMAXCONN );
    virtual std::unique_ptr<tcp_socket> accept();

    //values of how = SHUT_RD, SHUT_WR, or SHUT_RDWR
    virtual ssize_t shutdown( int how );

    virtual ssize_t send( const void* data, size_t len, std::chrono::milliseconds millis );
    virtual ssize_t recv( void* data, size_t len, std::chrono::milliseconds millis );

    virtual ssize_t can_send_data( std::chrono::milliseconds millis );
    virtual ssize_t can_recv_data( std::chrono::milliseconds millis );
    
    virtual std::pair<std::string, uint16_t> get_peer_address();
    virtual std::pair<std::string, uint16_t> get_local_address();

protected:

    std::pair<std::string, uint16_t> address_helper( struct sockaddr_storage* s );
    ssize_t create_socket( int address_family );

    size_t sockfd_;
};

};
#endif
