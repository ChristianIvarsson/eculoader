#ifndef TOOLS_H
#define TOOLS_H

#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <chrono>

#include "file/file.h"
#include "logger/logger.h"
#include "timer/timer.h"
#include "checksum/checksum.h"
#include "typedef.h"

extern std::string toString(const char* a);

extern u8  asc2u8(const char *ptr);
extern u16 asc2u16(const char *ptr);
extern u32 asc2u24(const char *ptr);
extern u32 asc2u32(const char *ptr);

extern size_t openRead( const char *fName, FILE **fp );
extern bool openWrite( const char *fName, FILE **fp );

// Because, F*CK C++..
// They can not hande 1-byte integers for some weird reason!
template< typename T >
std::string to_hex( T i )
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(sizeof(T)*2) << i;
    return stream.str();
}

template< typename T >
std::string to_hex( T i, uint32_t size)
{
    std::stringstream stream;
    if (size) stream << std::hex << std::setfill ('0') << std::setw(size*2) << i;
    else      stream << std::hex << i;
    return stream.str();
}

template< typename T >
std::string to_hex24( T i)
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(6) << i;
    return stream.str();
}

template< typename T >
std::string to_hex12( T i)
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
    uint8_t  *dat;
    size_t    len;
} lzCompData_t;

typedef struct {
    const uint8_t *src;
    uint8_t       *dst;
    size_t         len;
} lzData_t;

class lzcomp
{
    // Implement ring buffer of variable size
    volatile size_t index, count;
    volatile bool threadAct;

    volatile size_t queueSize;
    lzData_t * volatile queue;

    std::mutex mtx;

public:
    explicit lzcomp();
    ~lzcomp();

    // Push a request
    bool push(lzData_t *src);

    // The moment something has been popped, it's up to you to perform a delete[] on the returned compressed data.
    // Unpopped data is cleaned automatically upon destruction, however
    bool pop(lzData_t *src);

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