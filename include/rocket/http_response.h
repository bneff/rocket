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
#ifndef rocket_http_response
#define rocket_http_response

#include <stdio.h>
#include <string>
#include <string.h>     //strerror
#include <algorithm>    //find()
#include <map>          //multimap
#include <list>

#include "rocket/string_helpers.h"


namespace rocket{


enum class HTTPIO : ssize_t
{
    ERROR = -10,
    NEED_IO = 0,
    EMPTY_LINE = 4,
    END_OF_STARTLINE = 6,
    END_OF_HEADER = 8,
    END_OF_CHUNK = 10,
    END_OF_BODY = 12,
};
    

class http_response
{
public:


    //Could pass all of these into constructor to let caller decide max
    const size_t MAX_HEADER_SIZE = 4*1024;
    const size_t MAX_BODY_LENGTH = 1024*1024*32; //32MB Max
    const size_t INITIAL_STORAGE_SIZE = 128*1024;

    http_response()
    {
    }
    ~http_response()
    {}

    //This method will read an entire response, so a large file make take some time.
    ssize_t read_response( rocket::tcp_socket* socket, std::chrono::milliseconds timeout )
    {
        std::string prev_header;
        size_t bytes_received = 0;
        size_t curr_position = 0;
        size_t body_start = 0;
        
        storage_.clear();
        storage_.resize( INITIAL_STORAGE_SIZE );
        while( socket->can_recv_data( timeout ) )
        {
            if( storage_.capacity() - bytes_received < 1024 )
            {
                size_t new_size = storage_.capacity() + 128*1024;
                printf("re-sizing storage to %zu bytes\n", new_size);
                storage_.resize( new_size );
            }

            ssize_t bytes = socket->recv( storage_.data() + bytes_received, storage_.capacity()-bytes_received, timeout );

            if( bytes > 0 )
            {
                bytes_received += bytes;

                if( curr_position == 0 )
                {
                    HTTPIO ret = process_status_line( storage_.data(), storage_.data() + bytes_received, curr_position );

                    if( ret == HTTPIO::NEED_IO )
                        continue;
                    else if( ret < HTTPIO::NEED_IO )
                        break;
                }

                if( body_start == 0 )
                {
                    HTTPIO ret = HTTPIO::ERROR;
                    do
                    {
                        ret = process_header_line( storage_.data() + curr_position, storage_.data() + bytes_received, curr_position );
                    }
                    while( ret > HTTPIO::EMPTY_LINE );

                    if( ret == HTTPIO::NEED_IO )
                        continue;
                    else if( ret < HTTPIO::NEED_IO )
                        break;

                    body_start = curr_position;
                }

                if( body_start > 0 )
                {
                    if( headers.find("transfer-encoding") != headers.end() )
                    {
                        HTTPIO ret = HTTPIO::ERROR;
                        do
                        {
                            ret = process_chunked_body( storage_.data() + curr_position, storage_.data()+bytes_received, curr_position);
                        }
                        while( ret > HTTPIO::NEED_IO && ret < HTTPIO::END_OF_BODY );
                        //The above is > NEED_IO as some chunked implementations add extra empty lines before the 0 chunk
                        if( ret == HTTPIO::NEED_IO )
                            continue;
                        else if( ret < HTTPIO::NEED_IO )
                            break;

                        //////// CHECK FOR TRAILING HEADERS ///////////
                        if( bytes_received > curr_position )
                        {
                            ret = HTTPIO::ERROR;
                            do
                            {
                                ret = process_header_line( storage_.data() + curr_position, storage_.data() + bytes_received, curr_position );
                            }
                            while( ret > HTTPIO::EMPTY_LINE );

                            //If process_header_line returns NEED_IO, we could also be at EOF so we're done.
                            if( ret == HTTPIO::NEED_IO || ret == HTTPIO::EMPTY_LINE )
                                break;
                            else if( ret < HTTPIO::NEED_IO ) //ERROR
                                break;
                        }
                        else
                        {
                            //Reached end of data - we're done
                            break;
                        }
                    }
                    else
                    {
                        HTTPIO ret = process_body( storage_.data() + curr_position, storage_.data()+bytes_received, curr_position);
                        if( ret > HTTPIO::EMPTY_LINE )
                            break;
                        if( ret == HTTPIO::NEED_IO )
                            continue;
                        else if( ret == HTTPIO::ERROR )
                            break;
                    }
                }
            }
            else if( bytes == 0 )
            {
                printf("peer closed connection\n");
                break;
            }
            else
            {
                printf("socket error\n %s", strerror(errno));
                break;
            }
        }
    }

    HTTPIO process_status_line( const char* start, const char* end, size_t& position )
    {
        auto statusline_end = std::find(start, end,'\n');
        ssize_t distance = (statusline_end - start) + 1;

        if( distance > MAX_HEADER_SIZE )
            return HTTPIO::ERROR;

        if( statusline_end != end )
        {
            auto version_end = std::find(start, statusline_end, ' ');
            version = string_helpers::trim(std::string(start, version_end ));
            auto code_end = std::find(version_end+1, statusline_end, ' ');
            status_code = std::stoi(std::string(version_end+1, code_end));
            reason = string_helpers::trim(std::string(code_end+1, statusline_end));
            position += distance;

            return HTTPIO::END_OF_STARTLINE;
        }
        else
        {
            return HTTPIO::NEED_IO;
        }
    }

    HTTPIO process_header_line( const char* start, const char* end, size_t& position )
    {
        auto header_end = std::find(start, end,'\n');
        ssize_t distance = (header_end - start)+1;

        if( distance > MAX_HEADER_SIZE )
            return HTTPIO::ERROR;

        if( header_end != end )
        {
            position += distance;
            if( std::isspace(start[0]) )
            {
                //end of header, \r\n
                if( distance <= 2 )
                    return HTTPIO::EMPTY_LINE;

                //If starts with space or \t, this is a continuation
                //and should be appended to the previous header value
                (*prev_item_).second += string_helpers::trim(std::string(start, header_end-1));
            }
            else if( distance > 2 )
            {
                auto header_name_end = std::find(start, header_end, ':');
                prev_item_ = headers.emplace_hint( prev_item_,
                                    string_helpers::trim(std::string(start, header_name_end)),
                                    //string_helpers::lowercase(string_helpers::trim(std::string(start, header_name_end))),
                                    string_helpers::trim(std::string(header_name_end+1, header_end-1)));

                return HTTPIO::END_OF_HEADER;
            }
        }
        else
        {
            return HTTPIO::NEED_IO;
        }
    }

    HTTPIO process_body( const char* start, const char* end, size_t& position )
    {

        auto cl_header = headers.find("content-length");
        if( cl_header != headers.end() )
        {
            size_t content_length = strtoul(cl_header->second.c_str(), NULL, 0);
            size_t curr_content_size = end - start;
            if( curr_content_size >= content_length )
            {
                body_offsets_.push_back( std::make_pair( position, position+curr_content_size) );
                position += curr_content_size;
                return HTTPIO::END_OF_BODY;
            }
            else
            {
                return HTTPIO::NEED_IO;
            }
        }

    }

    HTTPIO process_chunked_body( const char* start, const char* end, size_t& position )
    {
        auto size_end = std::find(start, end,'\n');
        if( size_end != end )
        {
            std::string chunk_size_str = string_helpers::trim(std::string(start, size_end));
            std::string extension;

            //This is just a \r\n line
            if( chunk_size_str.length() == 0 )
            {
                //Some chunked implementations send a newline before the 0 chunk.
                position += (size_end - start)+1;
                return HTTPIO::EMPTY_LINE;
            }

            //Parse the ;chunk-extension if any
            auto extension_start = std::find(start, size_end, ';');
            if( extension_start != size_end )
            {
                extension = string_helpers::trim(std::string(extension_start, size_end-1));
                chunk_size_str = std::string(start, extension_start);
            }

            //Check if all the chars are valid HEX
            if( std::all_of(chunk_size_str.begin(), chunk_size_str.end(), ::isxdigit) )
            {
                size_t chunk_size = std::stoul(chunk_size_str, nullptr, 16);
                
                //Last chunk, we are done
                if( chunk_size == 0 )
                {
                    position += (size_end - start)+1;
                    return HTTPIO::END_OF_BODY;
                }

                //Make sure there is no off-by-one here
                if( end - size_end < chunk_size )
                {
                    return HTTPIO::NEED_IO;
                }
                else
                {
                    auto chunk_end = std::find(size_end+chunk_size, end,'\n');
                    if( chunk_end != end )
                    {
                        size_t end_of_chunk = position+(chunk_end-start);
                        if( std::isspace(storage_[end_of_chunk]) )
                        {
                            --end_of_chunk;
                        }
                        body_offsets_.push_back( std::make_pair( position+(size_end - start)+1, end_of_chunk ) );
                        position += (chunk_end - start)+1;
                        return HTTPIO::END_OF_CHUNK;
                    }
                    else
                    {
                        return HTTPIO::NEED_IO;
                    }
                }
            }
            else
            {
                //Chunk chars are not valid hex
                return HTTPIO::ERROR;
            }
        }
        else
        {
            return HTTPIO::NEED_IO;
        }
    }

    ssize_t process_multipart_body( const char* start, const char* end )
    {
    }

    std::string get_body()
    {
        std::string body;
        for( auto offset : body_offsets_ )
        {
            body += std::string(storage_.data()+offset.first, storage_.data()+offset.second);
        }
        return std::move(body);
    }

    uint16_t status_code;
    std::string reason;
    std::string version;

    std::multimap<std::string, std::string, string_helpers::ci_less> headers;

private:


    std::vector<char> storage_;
    std::multimap<std::string, std::string>::iterator prev_item_ = headers.end();
    std::list< std::pair<size_t, size_t> > body_offsets_;
};
};
#endif
