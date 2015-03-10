#ifndef rocket_tls_socket
#define rocket_tls_socket

#include <mutex>

#include "rocket/tcp_socket.h"

#include "openssl/ssl.h"
#include "openssl/err.h"

extern "C"
{
    struct CRYPTO_dynlock_value
    {
        std::mutex mutex;
    };
}

namespace rocket
{

static std::mutex* openssl_mutexes_;
static std::mutex tls_init_lock;

class tls_init
{

public:

    tls_init()
    {
        std::lock_guard<std::mutex> lock(tls_init_lock);
        tls_init_lock.lock();
        SSL_load_error_strings();   // readable error messages
        SSL_library_init();         // initialize library
        openssl_mutexes_ = new std::mutex[CRYPTO_num_locks()];
        CRYPTO_set_locking_callback(&lock_cb);
    }

    virtual ~tls_init()
    {
        std::lock_guard<std::mutex> lock(tls_init_lock);
        ERR_free_strings();
        CRYPTO_set_locking_callback(NULL);
        delete [] openssl_mutexes_;
    }

    static void lock_cb(int mode, int n, const char* file, int line)
    {
        if( mode & CRYPTO_LOCK )
            openssl_mutexes_[n].lock();
        else
            openssl_mutexes_[n].unlock();
    }

    /*
    static struct CRYPTO_dynlock_value* dynlock_create(const char* file, int line);
    static void dynlock(int mode, struct CRYPTO_dynlock_value* lock, const char* file, int line);
    static void dynlock_destroy(struct CRYPTO_dynlock_value* lock, const char* file, int line);V
    */

private:

};


class tls_socket : public tcp_socket
{
public:

    tls_socket(const tls_socket&) = delete;
    tls_socket& operator=(const tls_socket&) = delete;

    tls_socket(tls_socket&& other);
    tls_socket& operator=(tls_socket&& other);

    tls_socket();
    virtual ~tls_socket();

    virtual ssize_t connect( std::string host, uint16_t port, std::chrono::milliseconds millis );
    virtual ssize_t close();
    virtual ssize_t bind( std::string host, uint16_t port );
    virtual tcp_socket accept();
    //values of how = SHUT_RD, SHUT_WR, or SHUT_RDWR
    virtual ssize_t shutdown( int how );

    virtual ssize_t send( const void* data, size_t len, std::chrono::milliseconds millis );
    virtual ssize_t recv( void* data, size_t len, std::chrono::milliseconds millis );

    virtual ssize_t handle_error( ssize_t ssl_return_code );

    std::string hostname;
    bool verify_hostname = true;

private:

    SSL_CTX* ctx_ = nullptr;
    SSL* ssl_ = nullptr;
    std::mutex lock_;

};

};
#endif
