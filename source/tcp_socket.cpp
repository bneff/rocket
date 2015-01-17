#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <arpa/inet.h>
#include <utility>

#include <chrono>

#include "rocket/tcp_socket.h"

rocket::tcp_socket::tcp_socket() :
    sockfd_(0)
{
}

rocket::tcp_socket::~tcp_socket()
{
    //shutdown() will trigger an event and unblock any
    //current calls to can_recv_data and can_send_data
    shutdown( SHUT_WR );
    close();
}

rocket::tcp_socket::tcp_socket(rocket::tcp_socket&& other) :
    sockfd_(0)
{
    sockfd_ = other.sockfd_;
    other.sockfd_ = 0;
}

rocket::tcp_socket& rocket::tcp_socket::operator=(rocket::tcp_socket&& other)
{
    if( this != &other )
    {
        sockfd_ = other.sockfd_;
        other.sockfd_ = 0;
    }
    return *this;
}

ssize_t rocket::tcp_socket::close()
{
    if( sockfd_ > 0 )
    {
        //close can return an error, but on linux, even if it returns
        //non-zero, the fd has been closed.  So just ignore all errors.
        //FYI there is much reading on this subject - google it
        ::close(sockfd_);
        sockfd_ = 0;

        //Memory barrier to sync other threads in a send() or recv()
        __sync_synchronize;

    }
}

//Need to check for sockfd_ > 0 after poll() returns
//ssize_t rocket::tcp_socket::send
//ssize_t rocket::tcp_socket::recv

ssize_t rocket::tcp_socket::shutdown( int how )
{
    if( sockfd_ > 0 )
    {
        ::shutdown(sockfd_, how);
        printf("shutdown: %s\n", strerror(errno));
    }
}

ssize_t rocket::tcp_socket::bind(std::string host, uint16_t port )
{

    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //Let the OS figure it out for us
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //Fill in our ip in res to use in bind()

    if ((status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    if( sockfd_ <= 0 )
    {
        sockfd_ = socket(res->ai_family, SOCK_STREAM, 0);
        int err_socket = errno;
        if( sockfd_ <= 0 )
        {
            printf("Error: %s\n", strerror(err_socket));
        }
    }

    int optval = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    ssize_t ret = ::bind(sockfd_, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if( ret != 0 )
        printf("Error: %s\n", strerror(errno));
    return ret;
}

ssize_t rocket::tcp_socket::listen( size_t max_connections )
{
    ssize_t ret = ::listen(sockfd_, max_connections);
    if( ret != 0 )
        printf("Error: %s\n", strerror(errno));
    return ret;
}

rocket::tcp_socket rocket::tcp_socket::accept()
{
    struct sockaddr_storage remote_peer;
    socklen_t addr_len = sizeof(remote_peer);
    rocket::tcp_socket client_socket;

    // -1 will wait forever, turning this into a blocking call
    if( can_recv_data(std::chrono::milliseconds(-1) ) )
    {
        ssize_t ret = ::accept(sockfd_, (struct sockaddr *)&remote_peer, &addr_len);
        if( ret > 0 )
        {
            client_socket.sockfd_ = ret;
        }
    }
    return std::move(client_socket);
}

std::pair<std::string, uint16_t> rocket::tcp_socket::get_peer_address()
{
    //Don't call this on unconnected sockets e.g. server sockets
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_storage remote_peer;
    socklen_t addr_len = sizeof(remote_peer);

    if( getpeername(sockfd_, (struct sockaddr*)&remote_peer, &addr_len) == 0 )
    {
        return address_helper( &remote_peer );
    }
    else
    {
        printf("Error: %s\n", strerror(errno));
        return std::make_pair("",0);
    }
}

std::pair<std::string, uint16_t> rocket::tcp_socket::get_local_address()
{
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_storage local;
    socklen_t addr_len = sizeof(local);

    if( getsockname(sockfd_, (struct sockaddr*)&local, &addr_len) == 0 )
    {
        return address_helper( &local );
    }
    else
    {
        printf("Error: %s\n", strerror(errno));
        return std::make_pair("",0);
    }
}

ssize_t rocket::tcp_socket::connect( std::string host, uint16_t port, std::chrono::milliseconds millis )
{
    //connect
}

ssize_t rocket::tcp_socket::can_send_data( std::chrono::milliseconds millis )
{
    struct pollfd fds[1];
    fds[0].fd = sockfd_;
    fds[0].events = POLLOUT;
    fds[0].revents = 0;

    printf("Waiting for an event on the socket\n");
    int ret = poll(fds, 1, millis.count() );
    int poll_errno = errno;
    printf("Received write event on socket [%d]\n", ret);
    
    //if ret == 0, we timed out in poll
    //if ret == 1, then an event occured on our fd and we need to check it.
    //if ret == -1, then some error occured.  Check errno
    //if the remote side shut down their socket, or connect() doesn't complete, POLLHUP will be active.
    //if( fds[0].revents & ( POLLERR | POLLHUP | POLLNVAL) )

    if( ret == -1 )
    {
        printf("Error: %s\n", strerror(poll_errno));
        return -1;
    }
    if( fds[0].revents & POLLHUP )
    {
        printf("no connection to remote host\n");
        return -1;
    }
    else
    {
        return fds[0].revents & POLLOUT;
    }
}


ssize_t rocket::tcp_socket::can_recv_data( std::chrono::milliseconds millis )
{
    struct pollfd fds[1];
    fds[0].fd = sockfd_;
    fds[0].events = POLLIN | POLLPRI;
    fds[0].revents = 0;

    printf("Waiting for an event on the socket\n");
    int ret = poll(fds, 1, millis.count());
    int poll_errno = errno;
    printf("Received read event on socket [%d]\n", ret);


    //We might want to add logic to check revents for POLLERR, POLLHUP, and POLLNVAL and throw appropriately

    if( ret == -1 )
    {
        printf("Error: %s\n", strerror(poll_errno));
        return ret;
    }
    else
    {
        //Even if the socket has been closed, we want to return if there is data in the buffer.
        //NOTE: On Linux POLLIN will be set if the socket is closed and there is no data in the buffer
        //A subsequent recv call will return 0, and errno can be checked....
        //If POLLIN is not set, this returns 0, same as a timeout.
        return fds[0].revents & POLLIN;
    }
}

std::pair<std::string, uint16_t> rocket::tcp_socket::address_helper( struct sockaddr_storage* storage )
{
    char s[INET6_ADDRSTRLEN];
    uint16_t port = 0;
    void* tmp = nullptr;
    switch(storage->ss_family)
    {
        case AF_INET:
            tmp = &(((struct sockaddr_in *)storage)->sin_addr);
            port = ntohs(((struct sockaddr_in *)storage)->sin_port);
            break;
        case AF_INET6:
            tmp = &(((struct sockaddr_in6 *)storage)->sin6_addr);
            port = ntohs(((struct sockaddr_in6 *)storage)->sin6_port);
            break;

        default:
            printf("Unknown ss_family in get_peer_address()\n");
            return std::make_pair("",0);
    }
    std::string ip = inet_ntop(storage->ss_family, tmp, s, sizeof(s) );
    return std::make_pair(ip, port);
}
