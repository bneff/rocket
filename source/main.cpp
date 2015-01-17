#include <stdio.h>
#include <string>

#include "rocket/tcp_socket.h"

int main(int argc, char *argv[])
{
    rocket::tcp_socket socket;
    //socket.bind("::", 0);
    socket.bind("::", 1234);
    auto my_addr = socket.get_local_address();
    printf("Socket bound to %s %d\n", my_addr.first.c_str(), my_addr.second );
    socket.listen();
    while( true )
    {
        rocket::tcp_socket client = socket.accept();
        auto peer = client.get_peer_address();
        printf("Accepted connection from %s : %d\n", peer.first.c_str(), peer.second );
        client.can_recv_data(10000);
    }
    return 0;
}
