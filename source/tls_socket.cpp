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
#include <unistd.h> //close
#include <string>
#include <string.h>
#include <sys/fcntl.h>
#include <netdb.h> //freeaddrinfo
#include <chrono>

#include "rocket/tls_socket.h"

#include "x509v3/x509v3.h"
#include "openssl/err.h"

namespace rocket
{
    //define the externs from tls_socket.h
    SSL_CTX* ctx_ = SSL_CTX_new( SSLv23_method() );
    std::mutex* openssl_mutexes_ = new std::mutex[CRYPTO_num_locks()];
}


rocket::tls_socket::tls_socket() :
    tcp_socket()
{
}

rocket::tls_socket::~tls_socket()
{
    close();
}

rocket::tls_socket::tls_socket(rocket::tls_socket&& other)
{
    std::lock_guard<std::mutex>(other.lock_);
    sockfd_ = other.sockfd_;
    other.sockfd_ = 0;
}

rocket::tls_socket& rocket::tls_socket::operator=(rocket::tls_socket&& other)
{
    if( this != &other )
    {
        std::lock_guard<std::mutex>(other.lock_);
        sockfd_ = other.sockfd_;
        other.sockfd_ = 0;
    }
    return *this;
}

ssize_t rocket::tls_socket::close()
{
    std::lock_guard<std::mutex> guard(lock_);
    if( ssl_ )
    {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    tcp_socket::close(); //sockfd_ will be 0 after this call for all threads
}

ssize_t rocket::tls_socket::shutdown( int how )
{
    //Not protected by guard so another thread can break this out of a
    //send or recv.
    SSL_shutdown(ssl_);
    tcp_socket::shutdown(how);
}

ssize_t rocket::tls_socket::bind(std::string host, uint16_t port )
{
    std::lock_guard<std::mutex> guard(lock_);
    ssize_t ret = tcp_socket::bind( host, port );
    if( ret > 0 )
    {
        ssl_ = SSL_new(ctx_);
        SSL_set_fd( ssl_, sockfd_ );
    }
    return ret;
}

std::unique_ptr<rocket::tcp_socket> rocket::tls_socket::accept()
{
    std::lock_guard<std::mutex> guard(lock_);
    std::unique_ptr<tls_socket> client_socket(new tls_socket);
    std::unique_ptr<tcp_socket> tmp = tcp_socket::accept();
    if( tmp->sockfd_ > 0 )
    {
        client_socket->sockfd_ = tmp->sockfd_;
        client_socket->ssl_ = SSL_new( ctx_ );

        SSL_set_fd( client_socket->ssl_, client_socket->sockfd_);
        SSL_set_accept_state( client_socket->ssl_ );
    }
    tmp->sockfd_ = 0;
    return std::move(client_socket);
}

ssize_t rocket::tls_socket::connect( std::string host, uint16_t port, std::chrono::milliseconds millis )
{
    std::lock_guard<std::mutex> guard(lock_);

    ssize_t ret = tcp_socket::connect( host, port, millis );
    printf("RET %zu\n", ret );
    if( ret > 0 )
    {
        //NEED MORE ERROR CHECKING IN THIS SECTION
        //This method is too long...
        if( ssl_ == nullptr )
        {
            ssl_ = SSL_new( ctx_ );
        }
        SSL_set_fd(ssl_, sockfd_);

        if( hostname.empty() )
        {
            SSL_set_tlsext_host_name(ssl_, host.c_str());
        }
        else
        {
            SSL_set_tlsext_host_name(ssl_, hostname.c_str());
        }

        if( verify_hostname )
        {
            ssize_t ssl_ret = -1;
            ssize_t error = 0;
            do
            {
                ssl_ret = SSL_connect( ssl_ );
                error = handle_error( ssl_ret );
                if( error == SSL_ERROR_WANT_READ )
                {
                    can_recv_data( millis );
                }
                else if( error == SSL_ERROR_WANT_WRITE )
                {
                    can_send_data( millis );
                }
            }
            while( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );

            if( error != SSL_ERROR_NONE )
            {
                ret = 0;
            }

            if( ssl_ret == 1 )
            {
                X509 *cert = SSL_get_peer_certificate( ssl_ );
                if( cert != NULL )
                {
                    if( SSL_get_verify_result( ssl_ ) == X509_V_OK )
                    {
                        //If check is 1, the certificate matched the hostname
                        if( X509_check_host(cert, hostname.c_str(), 0, 0, NULL) != 1 )
                        {
                            printf("certificate does not match hostname\n");
                            ret = 0;
                        }
                    }
                    else
                    {
                        printf("unable to verify certificate presented by host\n");
                        ret = 0;
                    }
                    X509_free( cert );
                }
                else
                {
                    printf("no certificate returned from server\n");
                    ret = 0;
                }
            }
        }
        else
        {
            //passive connect method, not sure if I'm keeping this...
            SSL_set_connect_state( ssl_ );
        }
    }
    if( ret == 0 )
    {
        tcp_socket::shutdown(SHUT_RDWR);
        close(); //also closes underlying socket
    }
    return ret;
}

ssize_t rocket::tls_socket::send( const void* data, size_t len, std::chrono::milliseconds millis )
{
    std::lock_guard<std::mutex> guard(lock_);
    ssize_t ssl_ret = -1;
    ssize_t error = 0;
    do
    {
        ssl_ret = SSL_write( ssl_, data, len );
        error = handle_error( ssl_ret );
        if( error == SSL_ERROR_WANT_READ )
        {
            can_recv_data( millis );
        }
        else if( error == SSL_ERROR_WANT_WRITE )
        {
            can_send_data( millis );
        }
    }
    while( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );
    return ssl_ret;
}

ssize_t rocket::tls_socket::recv( void* data, size_t len, std::chrono::milliseconds millis )
{
    std::lock_guard<std::mutex> guard(lock_);
    ssize_t ssl_ret = -1;
    ssize_t error = 0;
    do
    {
        ssl_ret = SSL_read( ssl_, data, len );
        error = handle_error( ssl_ret );
        if( error == SSL_ERROR_WANT_READ )
        {
            //printf("waiting to read\n");
            can_recv_data( millis );
        }
        else if( error == SSL_ERROR_WANT_WRITE )
        {
            //printf("waiting to write\n");
            can_send_data( millis );
        }
    }
    while( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );
    return ssl_ret;
}

ssize_t rocket::tls_socket::handle_error( ssize_t ssl_return_code )
{
    ssize_t ssl_error = SSL_get_error(ssl_, ssl_return_code);

    //Pops errors off of the openssl error queue
    unsigned long error = ERR_get_error();
    while( error != 0 )
    {
        printf("SSL ERROR: %s\n", ERR_error_string(error, NULL));
        error = ERR_get_error();
    }

    switch( ssl_error )
    {
        case SSL_ERROR_NONE:
            //printf("ERROR NONE\n");
            break;
        case SSL_ERROR_WANT_WRITE:
            //printf("ERROR WANT WRITE\n");
            break;
        case SSL_ERROR_WANT_READ:
            //printf("ERROR WANT READ\n");
            break;
        case SSL_ERROR_SSL:
            printf("ERROR SSL\n");
            break;
        case SSL_ERROR_SYSCALL:
            //SYSCALL CHECK ERRNO
            printf("ERROR SYSCALL\n");
            break;
        case SSL_ERROR_ZERO_RETURN:
            //SSL SESSION WAS CLOSED
            printf("ERROR ZERO RETURN\n");
            break;
    }
    return ssl_error;
}

ssize_t rocket::tls_socket::private_key( std::string path )
{
    if( SSL_CTX_use_RSAPrivateKey_file( ctx_, path.c_str(), SSL_FILETYPE_PEM ) != 1 )
        printf("WTF PRIVATE\n");
}

ssize_t rocket::tls_socket::public_key( std::string path )
{
    //if( SSL_CTX_use_certificate_file( ctx_, path.c_str(), SSL_FILETYPE_PEM ) != 1 )
    if( SSL_CTX_use_certificate_chain_file( ctx_, path.c_str() ) != 1 )
        printf("WTF PUBLIC\n");
}

std::string rocket::tls_socket::tls_info()
{
    const SSL_CIPHER* cipher;
    cipher = SSL_get_current_cipher( ssl_ );

    char buf[512];
    SSL_CIPHER_description( cipher, buf, sizeof(buf) );

    //int alg_bits = 0;
    //int cipher_bits = 0;
    //cipher_bits = SSL_CIPHER_get_bits( cipher, &alg_bits );

    return std::string(buf);
}


//////////////////////////////////////////////////////
//
//This is an example of doing SSL_accept so we can read the SNI
//information from clients.  We could also use this for NPN info
//
//NEED ALL OF THIS IF WE ARE GOING TO PARSE THE SNI
//OTHERWISE SSL_get_servername will return null
/*
    ssize_t ssl_ret = -1;
    ssize_t error = 0;
    do
    {
        ssl_ret = SSL_accept( client_socket->ssl_ );
        error = client_socket->handle_error( ssl_ret );
        if( error == SSL_ERROR_WANT_READ )
        {
            client_socket->can_recv_data( std::chrono::milliseconds(10) );
        }
        else if( error == SSL_ERROR_WANT_WRITE )
        {
            client_socket->can_send_data( std::chrono::milliseconds(10) );
        }
    }
    while( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );
    if( error != SSL_ERROR_NONE )
    {
        printf("ERROR\n");
    }
    printf("SERVER NAME: %s\n", SSL_get_servername(client_socket->ssl_, TLSEXT_NAMETYPE_host_name) );
*/
//
//
//
/////////////////////////////////////////////////////
