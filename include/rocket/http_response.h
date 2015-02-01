#ifndef rocket_http_response
#define rocket_http_response

#include <stdio.h>
#include <string>
#include <string.h> //strerror
#include <algorithm> //find()

#include "rocket/string_helpers.h"

namespace rocket{

class http_response
{
public:
    
    const size_t MAX_HEADER_SIZE = 4*1024;
    //Could pass this into constructor
    const size_t INITIAL_STORAGE_SIZE = 128*1024;
    http_response()
    {
        storage_.resize( 10*1024 );
        storage_.resize( INITIAL_STORAGE_SIZE );
    }
    ~http_response()
    {}

    bool CheckMaxHeaderSize( size_t start, size_t end )
    {
        return (end-start) > MAX_HEADER_SIZE;
    }
    //This method will read an entire response, so a large file make take some time.
    //There will be a cap at N MBs
    //tcp_socket is not copyable to must use shared_ptr
    ssize_t read_response( std::shared_ptr<rocket::tcp_socket> socket, std::chrono::milliseconds timeout )
    {

        //1. Parse the Status-Line e.g. 'HTTP/1.1 200 OK'
        //2. Parse the end of headers which would be \r\n\r\n
        //3. Look for content-length, or chunked encoding
        //4. Parse the respose body

        size_t bytes_received = 0;
        size_t curr_position = 0;
        while( socket->can_recv_data( std::chrono::seconds(10) ) )
        {
            if( storage_.capacity() - bytes_received < 1024 )
            {
                size_t new_size = storage_.capacity() + 128*1024;
                printf("re-sizing storage to %d bytes\n", new_size);
                storage_.resize( new_size );
            }

            ssize_t bytes = socket->recv( storage_.data() + bytes_received, storage_.capacity()-bytes_received, timeout );

            if( bytes > 0 )
            {
                printf("BYTES %d\n", bytes );
                bytes_received += bytes;

                auto start_it = storage_.begin() + curr_position;
                auto end_it = storage_.begin() + bytes_received;

                if( curr_position == 0 )
                {
                    ssize_t ret = process_status_line( start_it, end_it );
                    if( ret > 0 )
                    {
                        curr_position += ret + 1;
                        start_it = storage_.begin() + curr_position;
                    }
                    else if( ret == 0 )
                        continue;
                    else
                        break;
                }

                //If ret =< 2, then we may not have any headers, just \r\n
                printf("DISTANCE SO FAR %d\n", std::distance(storage_.begin(), start_it) );
                ssize_t ret = process_header_line( start_it, end_it );
                while( ret > 2 )
                {
                    curr_position += ret + 1;
                    start_it = storage_.begin() + curr_position;
                    printf("DISTANCE SO FAR %d\n", std::distance(storage_.begin(), start_it) );
                    ret = process_header_line( start_it, end_it );
                }
                if( ret == 0 )
                    continue;
                else if( ret < 0 )
                    break;

                //AFTER THIS POINT WE ARE DONE WITH HEADERS

                curr_position += ret;
                start_it = storage_.begin() + curr_position;
                std::string body(start_it+1, end_it);
                printf("BYTES IN BUFFER %d\n", std::distance(start_it+1, end_it) );
                printf("BODY(%d): \n\"%s\"\n", body.size(), body.c_str() );
                

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

    ssize_t process_status_line( std::vector<char>::iterator start_it, std::vector<char>::iterator end_it )
    {

        auto statusline_end_it = std::find(start_it, end_it,'\n');
        ssize_t distance = std::distance(start_it, statusline_end_it);
        if( distance > MAX_HEADER_SIZE )
            return -1;

        if( statusline_end_it != end_it )
        {
            auto version_end = std::find(start_it, statusline_end_it, ' ');
            version = string_helpers::strip(std::string(start_it, version_end ));
            auto code_end = std::find(version_end+1, statusline_end_it, ' ');
            status_code = std::stoi(std::string(version_end+1, code_end));
            reason = string_helpers::strip(std::string(code_end+1, statusline_end_it));

            printf("\"%s %d %s\"\n", version.c_str(), status_code, reason.c_str() );
            return distance;
        }
        else
        {
            return 0;
        }
    }

    ssize_t process_header_line( std::vector<char>::iterator start_it, std::vector<char>::iterator end_it )
    {


        auto header_end_it = std::find(start_it, end_it,'\n');
        ssize_t distance = std::distance(start_it, header_end_it);
        printf("DISTANCE TO NEXT \\n: %d\n", distance );

        if( distance > MAX_HEADER_SIZE )
            return -1;

        if( header_end_it != end_it )
        {
            if( distance <= 2 )
            {
                //End of headers
                printf("START OF BODY\n");
                return distance;
            }

            auto header_name_end = std::find(start_it, header_end_it, ':');
            std::string key(start_it, header_name_end);
            //If this lines start with a space or tab then this should be appened to the previous header
            /*
            if( key.find_first_not_of(" \t") != 0 )
            {
                printf("line is a continuation from previous header line\n");
                //Search for current header key in map, strip and append the line
            }
            */

            std::string value(header_name_end+1, header_end_it-1 );
            printf("Key: \"%s\" Value: \"%s\"\n", string_helpers::trim(key).c_str(), string_helpers::strip(value).c_str() );
            return distance;
        }
        else
        {
            return 0;
        }
    }

    uint16_t status_code;
    std::string reason;
    std::string version;

private:

    std::vector<char> storage_;

};
};
#endif
