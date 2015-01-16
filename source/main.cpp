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

#include "rocket/socket.h"

int main(int argc, char *argv[])
{
    rocket::tcp_socket socket;
    socket.bind("::", 1234);
    socket.listen(SOMAXCONN);
    while( true )
    {
        rocket::tcp_socket client = socket.accept();
        auto peer = client.get_peer_address();
        printf("Accepted connection from %s : %d\n", peer.first.c_str(), peer.second );
    }
    return 0;
}
