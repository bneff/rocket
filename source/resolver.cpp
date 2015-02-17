#include <string>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "rocket/resolver.h"

rocket::resolver::resolver( int family )
{
    family_ = family;
}

rocket::resolver::~resolver()
{
}

std::vector<std::string> rocket::resolver::resolve( std::string hostname, std::string service, int family, std::string& error_string )
{
    std::vector<std::string> results;

    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family; // AF_INET,AF_INET6,AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    if ((status = getaddrinfo(hostname.c_str(), service.c_str(), &hints, &res)) != 0)
    {
        error_string = gai_strerror(status);
        return results;
    }

    for(p = res;p != NULL; p = p->ai_next)
    {
        void *addr;
        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        results.push_back( ipstr );
    }
    freeaddrinfo(res);
    return results;
}
