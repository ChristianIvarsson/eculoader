#ifndef TOOLS_H
#define TOOLS_H

#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>

#include "file/file.h"
#include "logger/logger.h"
#include "timer/timer.h"
#include "checksum/checksum.h"
#include "checksum/md5.h"
#include "partition_helper.h"

#include "typedef.h"

extern std::string toString(const char* a);

extern u8  asc2u8(const char *ptr);
extern u16 asc2u16(const char *ptr);
extern u32 asc2u24(const char *ptr);
extern u32 asc2u32(const char *ptr);

extern size_t openRead( const char *fName, FILE **fp );
extern bool openWrite( const char *fName, FILE **fp );

template< typename T >
std::string to_hex(T i)
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(sizeof(T)*2) << i;
    return stream.str();
}

template< typename T >
std::string to_hex(T i, uint32_t size)
{
    std::stringstream stream;
    if (size) stream << std::hex << std::setfill ('0') << std::setw(size*2) << i;
    else      stream << std::hex << i;
    return stream.str();
}

template< typename T >
std::string to_hex24(T i)
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(6) << i;
    return stream.str();
}

template< typename T >
std::string to_hex12(T i)
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(3) << i;
    return stream.str();
}

template <typename T>
std::string to_string_fcount(const T a_value, const int32_t n = 2)
{
    std::ostringstream out;
    out << std::setfill('0') << std::setw(n) << a_value;
    return out.str();
}

typedef struct {
    uint8_t *data;
    size_t   length;
} lzData_t;

class lzcomp
{
    typedef struct {
        lzData_t buffer;
        lzData_t lzData;
        int32_t  token;
    } lzQeueData_t;

    class lz_queue
    {
        std::mutex mtx;
        volatile size_t count;
        volatile size_t size;
        volatile lzQeueData_t * volatile queue;

        bool isPresent(int32_t token);

        uint32_t tokenCntr;

    public:
        explicit lz_queue()
        {
            size = count = 0;
            tokenCntr = 0;
            queue = nullptr;
        }

        ~lz_queue()
        {
            printf("queue death\n");
        }

        void nuke();
        void flush();

        bool push(uint8_t *, const size_t &, int32_t &);
        bool popLz(volatile bool &, lzData_t &, const int32_t &);
        bool getLz(volatile bool &, lzData_t &, const int32_t &);

        // Get first item where lsData has not yet been set
        bool waiting(lzQeueData_t &);
        // Update item with lzdata.
        bool update(const lzQeueData_t &, uint8_t *, const size_t &);
    };

    lz_queue reqQueue;

    // Compress thread
    std::mutex mtxThread;
    std::condition_variable threadCondition;
    volatile bool runThread;
    std::thread tFunc;

    bool startThread();
    bool stopThread();
    void pumpThread();
    void lzThread();

    std::mutex intMtx;

public:
    explicit lzcomp();
    ~lzcomp();

    // Push a request
    // Returned value:
    // >= 0 Token
    // < 0 Error
    int32_t push(const uint8_t *dat, size_t length);

    // The moment something has been popped, it's up to you to perform a delete[] on the returned compressed data.
    // Unpopped data is cleaned automatically upon destruction, however.
    lzData_t pop(int32_t token);

    // Retrieve data for block but keep it in the queue
    lzData_t get(int32_t token);

    void flush();
};

class stopWatch
{
    std::chrono::time_point<std::chrono::system_clock> oldTime, newTime;

public:
    explicit stopWatch()
    {
        newTime = oldTime = std::chrono::system_clock::now();
    }

    void capture()
    {
        auto tempTime = std::chrono::system_clock::now();
        oldTime = newTime;
        newTime = tempTime;
    }

    void print()
    {
        uint64_t msTaken = std::chrono::duration_cast<std::chrono::milliseconds>(newTime - oldTime).count();
        uint32_t secTaken = (msTaken /  1000) % 60;
        uint32_t minTaken = (msTaken / 60000) % 60;
        msTaken %= 1000;
        logger::log("Duration " + std::to_string(minTaken) + "m, " + std::to_string(secTaken) + "s, " + std::to_string(msTaken) + "ms");
    }
};

#endif