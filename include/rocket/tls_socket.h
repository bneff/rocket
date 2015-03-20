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
#ifndef rocket_tls_socket
#define rocket_tls_socket

#include <mutex>

#include "rocket/tcp_socket.h"

#include "openssl/ssl.h"



namespace rocket
{

class tls_socket : public tcp_socket
{
public:

    tls_socket(const tls_socket&) = delete;
    tls_socket& operator=(const tls_socket&) = delete;

    tls_socket(tls_socket&& other);
    tls_socket& operator=(tls_socket&& other);

    tls_socket();
    virtual ~tls_socket();

    virtual ssize_t connect( std::string host, uint16_t port, std::chrono::milliseconds millis );
    virtual ssize_t close();
    virtual ssize_t bind( std::string host, uint16_t port );
    virtual std::unique_ptr<tcp_socket> accept();
    virtual ssize_t shutdown( int how ); //values of how = SHUT_RD, SHUT_WR, or SHUT_RDWR

    virtual ssize_t send( const void* data, size_t len, std::chrono::milliseconds millis );
    virtual ssize_t recv( void* data, size_t len, std::chrono::milliseconds millis );

    virtual ssize_t handle_error( ssize_t ssl_return_code );

    ssize_t private_key( std::string path );
    ssize_t public_key( std::string path );

    std::string tls_info();

    std::string hostname;
    bool verify_hostname = true;

private:

    SSL* ssl_ = nullptr;
    std::mutex lock_;

};

};
#endif
