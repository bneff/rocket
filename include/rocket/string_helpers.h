/*
Copyright (c) 2015, Bryan Neff
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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



//from: http://stackoverflow.com/questions/1801892/making-mapfind-operation-case-insensitive
/************************************************************************/
/* Comparator for case-insensitive comparison in STL assos. containers  */
/************************************************************************/
struct ci_less : std::binary_function<std::string, std::string, bool>
{
// case-independent (ci) compare_less binary function
struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool>
{
  bool operator() (const unsigned char& c1, const unsigned char& c2) const {
      return tolower (c1) < tolower (c2);
  }
};
bool operator() (const std::string & s1, const std::string & s2) const {
  return std::lexicographical_compare
    (s1.begin (), s1.end (),   // source range
    s2.begin (), s2.end (),   // dest range
    nocase_compare ());  // comparison
}

};


};
#endif
