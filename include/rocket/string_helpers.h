#ifndef rocket_string_helpers
#define rocket_string_helpers


#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <string>

namespace string_helpers
{

//taken from: http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline std::string lowercase( std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// trim from start - removes '\r' ' ' '\n' & '\t'
static inline std::string ltrim(std::string s)
{
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end - removes '\r' ' ' '\n' & '\t'
static inline std::string rtrim(std::string s)
{
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string trim(std::string s)
{
        return ltrim(rtrim(s));
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems, bool strip )
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if( strip )
        {
            item.erase( std::remove(item.begin(), item.end(), '\r'), item.end() );
            item.erase( std::remove(item.begin(), item.end(), '\n'), item.end() );
        }
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems, true); return elems;
}

};
#endif
