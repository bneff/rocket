#include <stdio.h>
#include <unistd.h> //close
#include <string>
#include <string.h>
#include <sys/fcntl.h>
#include <netdb.h> //freeaddrinfo
#include <chrono>

#include "rocket/tls_socket.h"

#include "x509v3/x509v3.h"

rocket::tls_socket::tls_socket() :
    tcp_socket()
{
    ctx_ = SSL_CTX_new( SSLv23_method() );
    SSL_CTX_set_options( ctx_, SSL_OP_NO_SSLv2 );
    SSL_CTX_set_options( ctx_, SSL_OP_NO_SSLv3 );
    SSL_CTX_set_cipher_list(ctx_, "HIGH:MEDIUM");
    SSL_CTX_set_session_cache_mode(ctx_, SSL_SESS_CACHE_OFF);
    //THIS COULD FAIL. NEED ERROR HANDLING
    SSL_CTX_load_verify_locations(ctx_, "./3rdparty/ca-bundle.crt", NULL);
}

rocket::tls_socket::~tls_socket()
{
    std::lock_guard<std::mutex> guard(lock_);
    close();
    if( ctx_ )
        SSL_CTX_free( ctx_ );
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
    //Not protected by guard to another thread can break this out of a
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

rocket::tcp_socket rocket::tls_socket::accept()
{
    std::lock_guard<std::mutex> guard(lock_);
    tls_socket client_socket;
    tcp_socket tmp = tcp_socket::accept();
    if( tmp.sockfd_ > 0 )
    {
        client_socket.sockfd_ = tmp.sockfd_;
        client_socket.ssl_ = SSL_new( ctx_ );

        SSL_set_fd( client_socket.ssl_, client_socket.sockfd_);
        SSL_set_accept_state( client_socket.ssl_ );
    }
    tmp.sockfd_ = 0;
    return std::move(client_socket);
}

ssize_t rocket::tls_socket::connect( std::string host, uint16_t port, std::chrono::milliseconds millis )
{
    std::lock_guard<std::mutex> guard(lock_);

    ssize_t ret = tcp_socket::connect( host, port, millis );
    printf("RET %d\n", ret );
    if( ret > 0 )
    {
        //NEED MORE ERROR CHECKING IN THIS SECTION
        //This method is too long...
        if( ssl_ == nullptr )
        {
            ssl_ = SSL_new( ctx_ );
        }
        SSL_set_fd(ssl_, sockfd_);

        if( this->hostname.empty() )
        {
            SSL_set_tlsext_host_name( ssl_, host.c_str() );
        }
        else
        {
            SSL_set_tlsext_host_name( ssl_, this->hostname.c_str() );
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
                tcp_socket::shutdown(SHUT_RDWR);
                close(); //also closes underlying socket
                ret = 0;
            }

            if( ssl_ret == 1 )
            {
                X509 *cert = SSL_get_peer_certificate( ssl_ );
                if( cert != NULL )
                {
                    if( SSL_get_verify_result( ssl_ ) == X509_V_OK )
                    {
                        int check = X509_check_host(cert, hostname.c_str(), 0, 0, NULL);
                        //If check is 1, the certificate matched the hostname
                        if( check == 1 )
                        {
                            const SSL_CIPHER* cipher;
                            cipher = SSL_get_current_cipher( ssl_ );

                            char buf[512];
                            SSL_CIPHER_description( cipher, buf, sizeof(buf) );

                            int alg_bits = 0;
                            int cipher_bits = 0;
                            cipher_bits = SSL_CIPHER_get_bits( cipher, &alg_bits );

                            printf("Cipher Bits: %d  Algorithm Bits: %d\n", cipher_bits, alg_bits );
                            printf("VERSION: %s\nNAME: %s\nDESC: %s\n",
                            SSL_CIPHER_get_version( cipher  ),
                            SSL_CIPHER_get_name( cipher ),
                            buf);
                        }
                        else
                        {
                            printf("NOT VALID\n");
                            tcp_socket::shutdown(SHUT_RDWR);
                            tcp_socket::close();
                            ret = 0;
                        }
                    }
                    else
                    {
                        printf("unable to verify cert presented by host\n");
                    }
                    X509_free( cert );
                }
                else
                {
                    printf("no cert returned from server\n");
                }
            }
        }
        else
        {
            //passive connect method, not sure if I'm keeping this...
            SSL_set_connect_state( ssl_ );
        }
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
            printf("waiting to read\n");
            can_recv_data( millis );
        }
        else if( error == SSL_ERROR_WANT_WRITE )
        {
            printf("waiting to write\n");
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
