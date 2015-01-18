#ifndef rocket_resolver
#define rocket_resolver

#include <vector>

namespace rocket
{


class resolver
{
public:

    resolver();
    virtual ~resolver();
    
    static std::vector<std::string> resolve( std::string hostname, std::string service );


private:

};

};
#endif
