#ifndef rocket_tcp_socket
#define rocket_tcp_socket

//SOMAXCONN
#include <sys/socket.h>

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
    virtual tcp_socket accept();

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
