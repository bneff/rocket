#ifndef rocket_resolver
#define rocket_resolver

#include <vector>

namespace rocket
{


class resolver
{
public:

    resolver( int family = AF_UNSPEC );
    virtual ~resolver();
    
    static std::vector<std::string> resolve( std::string hostname, std::string service, int family=AF_UNSPEC );

private:

    int family_;
};

};
#endif
