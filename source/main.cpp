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
    socket.bind("::", 1234);  //Set port to 0 for ephemeral
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
            client.send(tmp, bytes_received, std::chrono::seconds(10));
            memset(tmp, 0, sizeof(tmp));
            bytes_received = client.recv(tmp, sizeof(tmp), std::chrono::seconds(10));
        }
        //client.can_recv_data(std::chrono::seconds(10));
    }
    return 0;
}
