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
#ifndef rocket_tls_init
#define rocket_tls_init

#include <mutex>
#include "openssl/ssl.h"
#include "openssl/err.h"

//TODO
//Impl TLS EXT SERVER SIDE
//1. Use SSL_get_servername(ssl_, TLSEXT_NAMETYPE_host_name) to figure out if the client is indicating TLS EXT
//2. Create a tls_init_server() that takes a "hostname" and keys to support server-side tls ext
//3. Call SSL_set_SSL_CTX() to set the accepted connection the appropriate CTX
//

namespace rocket
{

//ctx_ must be extern so each compliation unit 'sees' the proper value
//I'm guessing openssl_mutexes_ must be the same way...
extern std::mutex* openssl_mutexes_;
extern SSL_CTX* ctx_;
static std::mutex tls_init_lock;

class tls_init
{

public:

    tls_init()
    {
        std::lock_guard<std::mutex> lock(tls_init_lock);
        SSL_load_error_strings();   // readable error messages
        SSL_library_init();         // initialize library
        CRYPTO_set_locking_callback(&lock_cb);

        ctx_ = SSL_CTX_new( SSLv23_method() );
        SSL_CTX_set_options( ctx_, SSL_OP_NO_SSLv2 );
        SSL_CTX_set_options( ctx_, SSL_OP_NO_SSLv3 );
        SSL_CTX_set_cipher_list(ctx_, "HIGH:MEDIUM");
        SSL_CTX_set_session_cache_mode(ctx_, SSL_SESS_CACHE_OFF);
        //THIS COULD FAIL. NEED ERROR HANDLING
        SSL_CTX_load_verify_locations(ctx_, "./3rdparty/ca-bundle.crt", NULL);
    }

    ssize_t set_cipher_list( std::string cipher_list )
    {
        return SSL_CTX_set_cipher_list(ctx_, cipher_list.c_str());
    }

    ssize_t set_trust_bundle_location( std::string path )
    {
        return SSL_CTX_load_verify_locations(ctx_, path.c_str(), NULL);
    }

    ssize_t set_private_key( std::string path )
    {
        if( SSL_CTX_use_RSAPrivateKey_file( ctx_, path.c_str(), SSL_FILETYPE_PEM ) != 1 ) 
            printf("WTF PRIVATE\n");
        return 0;
    }

    ssize_t set_public_key( std::string path )
    {
        //if( SSL_CTX_use_certificate_file( ctx_, path.c_str(), SSL_FILETYPE_PEM ) != 1 )
        if( SSL_CTX_use_certificate_chain_file( ctx_, path.c_str() ) != 1 ) 
            printf("WTF PUBLIC\n");
        return 0;
    }

    virtual ~tls_init()
    {
        std::lock_guard<std::mutex> lock(tls_init_lock);
        ERR_free_strings();
        CRYPTO_set_locking_callback(NULL);
        delete [] openssl_mutexes_;

        if( ctx_ )
        {
            SSL_CTX_free( ctx_ );
        }
    }

    static void lock_cb(int mode, int n, const char* file, int line)
    {
        if( mode & CRYPTO_LOCK )
            openssl_mutexes_[n].lock();
        else
            openssl_mutexes_[n].unlock();
    }
};

};
#endif
