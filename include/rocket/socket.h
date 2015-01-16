#ifndef rocket_tcp_socket
#define rocket_tcp_socket
namespace rocket
{

class tcp_socket
{
public:

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

    tcp_socket(tcp_socket&& other);
    tcp_socket& operator=(tcp_socket&& other);

    //TODO: Timeouts should be a duration in cpp11
    
    tcp_socket();
    virtual ~tcp_socket();

    virtual ssize_t connect( std::string host, uint16_t port, int timeout );
    //virtual ssize_t close();
    virtual ssize_t bind( std::string host, uint16_t port );
    virtual ssize_t listen( size_t max_connections );
    virtual tcp_socket accept();
    //virtual ssize_t shutdown( int mode );

    //virtual size_t send( void* data, size_t len, int timeout );
    //virtual size_t recv( void* data, size_t len, int timeout );

    virtual ssize_t can_send_data( int timeout );
    virtual ssize_t can_recv_data( int timeout );
    
    virtual std::pair <std::string, uint16_t> get_peer_address();
    //virtual std::string get_local_address();

private:

    size_t sockfd_;
};

};
#endif
