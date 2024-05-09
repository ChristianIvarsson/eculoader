#include "tools.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace logger;

string toString(const char* a) 
{
    string s = "";
    while (*a)
        s += *a++;

    return s; 
}

u8 asc2u8(const char *ptr)
{
    u8 tmp1, tmp2;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr)   > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    return (u8)( (tmp1 << 4) | tmp2 );
}

u16 asc2u16(const char *ptr)
{
    u8  tmp1, tmp2, tmp3, tmp4;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr++) > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    tmp3 = ((tmp3 = (u8)*ptr++) > 64) ? (u8)(tmp3 + 201) : (u8)(tmp3 + 208);
    tmp4 = ((tmp4 = (u8)*ptr)   > 64) ? (u8)(tmp4 + 201) : (u8)(tmp4 + 208);
    return (u16)( (tmp1 << 12) | (tmp2 << 8) |
                  (tmp3 <<  4) | (tmp4) );
}

u32 asc2u24(const char *ptr)
{
    u8 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr++) > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    tmp3 = ((tmp3 = (u8)*ptr++) > 64) ? (u8)(tmp3 + 201) : (u8)(tmp3 + 208);
    tmp4 = ((tmp4 = (u8)*ptr++) > 64) ? (u8)(tmp4 + 201) : (u8)(tmp4 + 208);
    tmp5 = ((tmp5 = (u8)*ptr++) > 64) ? (u8)(tmp5 + 201) : (u8)(tmp5 + 208);
    tmp6 = ((tmp6 = (u8)*ptr)   > 64) ? (u8)(tmp6 + 201) : (u8)(tmp6 + 208);
    return (u32)( (tmp1 << 20) | (tmp2 << 16) |
                  (tmp3 << 12) | (tmp4 <<  8) |
                  (tmp5 <<  4) | (tmp6) );
}

u32 asc2u32(const char *ptr)
{
    u8 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr++) > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    tmp3 = ((tmp3 = (u8)*ptr++) > 64) ? (u8)(tmp3 + 201) : (u8)(tmp3 + 208);
    tmp4 = ((tmp4 = (u8)*ptr++) > 64) ? (u8)(tmp4 + 201) : (u8)(tmp4 + 208);
    tmp5 = ((tmp5 = (u8)*ptr++) > 64) ? (u8)(tmp5 + 201) : (u8)(tmp5 + 208);
    tmp6 = ((tmp6 = (u8)*ptr++) > 64) ? (u8)(tmp6 + 201) : (u8)(tmp6 + 208);
    tmp7 = ((tmp7 = (u8)*ptr++) > 64) ? (u8)(tmp7 + 201) : (u8)(tmp7 + 208);
    tmp8 = ((tmp8 = (u8)*ptr)   > 64) ? (u8)(tmp8 + 201) : (u8)(tmp8 + 208);
    return (u32)( (tmp1 << 28) | (tmp2 << 24) |
                  (tmp3 << 20) | (tmp4 << 16) |
                  (tmp5 << 12) | (tmp6 <<  8) |
                  (tmp7 <<  4) | (tmp8) );
}


#warning "Move this to a file class and make things neat"

size_t openRead( const char *fName, FILE **fp ) {

    if ( (*fp = fopen( fName, "rb" )) == nullptr ) {
        log("Error: openRead() - Could not get file handle");
        return 0;
    }

    fseek(*fp, 0L, SEEK_END);
    long fileSize = ftell(*fp);
    rewind(*fp);

    if ( fileSize < 0 ) {
        log("Error: openRead() - Unable to determine file size");
        fclose( *fp );
        return 0;
    }

    if ( fileSize == 0 ) {
        log("Error: openRead() - There's no data to read");
        fclose( *fp );
        return 0;
    }

    return (size_t)fileSize;
}

bool openWrite( const char *fName, FILE **fp ) {

    if ( (*fp = fopen( fName, "wb" )) == nullptr ) {
        log("Error: openWrite() - Could not get file handle");
        return false;
    }

    rewind( *fp );
    return true;
}

// Do NOT change!
#define LZ_MAX_OFFSET   ( 4096 )

static inline uint32_t LZ_StringCompare(
                            const uint8_t *str1,
                            const uint8_t *str2,
                            uint32_t minlen,
                            uint32_t maxlen)
{
    for ( ; (minlen < maxlen) && (str1[minlen] == str2[minlen]); minlen++);
    return minlen;
}

static uint32_t lzCompress(const uint8_t *in, const size_t insize, uint8_t *out)
{
    // Do we have anything to compress?
    if ( insize == 0 )
    {
        log(lzcomplog, "Can't compress 0 bytes");
        return 0;
    }

    // Start of compression
    uint8_t *flags = out;
    uint8_t mask = 0x80;

    int32_t bytesleft = insize;
    size_t  inpos = 0;
    size_t  outpos = 1;


    *flags = 0;

    // Main compression loop
    do {

        // Determine most distant position
        int32_t maxoffset = (int32_t) (inpos > LZ_MAX_OFFSET) ? LZ_MAX_OFFSET : inpos;

        // Get pointer to current position
        const uint8_t *ptr1 = &in[ inpos ];

        // Search history window for maximum length string match
        uint32_t bestlength = 2;
        uint32_t bestoffset = 0;

        for ( int32_t offset = 3; offset <= maxoffset; offset++ )
        {
            // Get pointer to candidate string
            const uint8_t *ptr2 = &ptr1[ -offset ];

            // Quickly determine if this is a candidate (for speed)
            if ( (ptr1[ 0 ] == ptr2[ 0 ] ) &&
                 (ptr1[ bestlength ] == ptr2[ bestlength ]) )
            {
                // Determine maximum length for this offset
                // Count maximum length match at this offset
                uint32_t length = LZ_StringCompare( ptr1, ptr2, 0, ((inpos + 1) > 18 ? 18 : inpos + 1) );

                // Better match than any previous match?
                if ( length > bestlength )
                {
                    bestlength = length;
                    bestoffset = offset;
                }
            }
        }

        // Was there a good enough match?
        if ( bestlength > 2 )
        {
            *flags |= mask;

            mask >>= 1;

            out[ outpos++ ] = (bestlength - 3) << 4 | (((bestoffset - 1) & 0xF00) >> 8);
            out[ outpos++ ] = (bestoffset - 1) & 0xFF;

            inpos += bestlength;
            bytesleft -= bestlength;

            if ( mask == 0 )
            {
                mask = 0x80;
                flags = &out[ outpos++ ];
                *flags = 0;
            }
        }
        else
        {
            mask >>= 1;
            out[ outpos++ ] = in[ inpos++ ];
            bytesleft--;

            if ( mask == 0 )
            {
                mask = 0x80;
                flags = &out[ outpos++ ];
                *flags = 0;
            }
        }

    } while ( bytesleft > 3 );

    // Dump remaining bytes, if any
    while ( inpos < insize )
    {
        mask >>= 1;
        out[ outpos++ ] = in[ inpos++ ];

        if ( mask == 0 )
        {
            mask = 0x80;
            flags = &out[ outpos++ ];
            *flags = 0;
        }
    }

    while ( (outpos & 3) != 0 )
    {
        out[ outpos++ ] = 0;
    }

    log(lzcomplog, "Inpos:  " + to_string(inpos));
    log(lzcomplog, "Outpos: " + to_string(outpos));

    return outpos;
}

lzcomp::lzcomp()
    : tFunc()
{
    log(lzcomplog, "lzcomp()");

    runThread = false;

    if ( !startThread() )
    {
        exit (1);
    }
}

lzcomp::~lzcomp()
{
    log(lzcomplog, "~lzcomp()");

    // - Could create a race cond. -
    intMtx.lock();

    stopThread();

    reqQueue.nuke();

    intMtx.unlock();
}

// Does not mutex lock.
bool lzcomp::startThread()
{
    if ( runThread )
    {
        log(lzcomplog, "Thread already running");
        return true;
    }

    tFunc = thread(&lzcomp::lzThread, this);

    // while (!runThread && !timeout) ?
    timer::sleepMilli( 100 );

    return true;
}

// Does not use mutex lock.
bool lzcomp::stopThread()
{
    runThread = false;

    pumpThread();

    if ( tFunc.joinable() )
    {
        log(lzcomplog, "Attempting thread join");
        tFunc.join();
    }

    return true;
}

// Does not use mutex lock.
void lzcomp::pumpThread()
{
    log(lzcomplog, "lz pump thread");

    lock_guard<mutex> mLock( mtxThread );
    threadCondition.notify_one();
}

void lzcomp::lzThread()
{
    runThread = true;

    log(lzcomplog, "lzThread up");

    // locking
    unique_lock<mutex> mLock( mtxThread );

    // TODO: This reeks of possible race conditions..

    while ( runThread )
    {
        lzQeueData_t request;

        if ( reqQueue.waiting( request ) )
        {
            // log(lzcomplog, "Start comp");
            // log(lzcomplog, "Size: " + to_string( request.buffer.length ));

            if ( request.buffer.data == nullptr )
            {
                log(lzcomplog, "Popped nullptr");
                exit( 1 );
            }

            // Compressed data can for sure be larger than input.
            // - Question is HOW much larger?
            uint8_t *dst = new uint8_t[ (request.buffer.length * 2) + 128 ];

            if ( dst == nullptr )
            {
                log(lzcomplog, "Could not allocate compressed buffer");
                exit( 1 );
            }

            size_t compLen = lzCompress( request.buffer.data, request.buffer.length, dst );

            if ( compLen > 0 )
            {
                if ( !reqQueue.update( request, dst, compLen ) )
                {
                    delete[] dst;
                    exit( 1 );
                }
            }
            else
            {
                log(lzcomplog, "No returned compressed data");
                delete[] dst;
                exit( 1 );
            }
        }
        else
        {
            // log(lzcomplog, "lz thread wait");
            // Wait
            threadCondition.wait( mLock );
            // log(lzcomplog, "lz thread step");
        }
    }

    mLock.unlock();

    log(lzcomplog, "lzThread down");
}

void lzcomp::flush()
{
    intMtx.lock();

    stopThread();

    reqQueue.flush();

    if ( !startThread() )
    {
        intMtx.unlock();
        exit (1);
    }

    intMtx.unlock();
}

int32_t lzcomp::push(const uint8_t *dat, size_t length)
{
    intMtx.lock();

    int32_t token = -1;

    if ( reqQueue.push((uint8_t*)dat, length, token) )
    {
        intMtx.unlock();

        pumpThread();

        return token;
    }

    intMtx.unlock();

    return token;
}

lzData_t lzcomp::pop(int32_t tokenToGet)
{
    lzData_t retVal = { 0 };

    reqQueue.popLz( runThread, retVal, tokenToGet );

    return retVal;
}

lzData_t lzcomp::get(int32_t tokenToGet)
{
    lzData_t retVal = { 0 };

    reqQueue.getLz( runThread, retVal, tokenToGet );

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// Queue functions
void lzcomp::lz_queue::flush()
{
    mtx.lock();

    if ( size == 0 || queue == nullptr )
    {
        mtx.unlock();
        return;
    }

    log(lzcomplog, "Flushing " + to_string(count) + " item(s)");

    // Implement try / catch to find bugs?

    for ( size_t i = 0; i < size; i++ )
    {
        if ( queue[ i ].lzData.data != nullptr )
        {
            log(lzcomplog, "<< Clearing queue data item >>");
            delete[] queue[ i ].lzData.data;
            queue[ i ].lzData.data = nullptr;
        }
    }

    memset((void*)queue, 0, size * sizeof(lzQeueData_t));

    for ( size_t i = 0; i < size; i++ )
    {
        queue[ i ].token = -1;
    }

    count = 0;

    mtx.unlock();
}

void lzcomp::lz_queue::nuke()
{
    mtx.lock();

    log(lzcomplog, "Nuking " + to_string(count) + " item(s)");

    if ( queue != nullptr )
    {
        // Implement try / catch to find bugs?

        for ( size_t i = 0; i < size; i++ )
        {
            if ( queue[ i ].lzData.data != nullptr )
            {
                log(lzcomplog, "<< Clearing queue data item >>");
                delete[] queue[ i ].lzData.data;
                queue[ i ].lzData.data = nullptr;
            }
        }

        free((void*)queue);
        queue = nullptr;
    }

    size = count = 0;

    mtx.unlock();
}

bool lzcomp::lz_queue::isPresent(int32_t token)
{
    if ( token < 0 )
    {
        return false;
    }

    mtx.lock();

    if ( queue == nullptr || size == 0 )
    {
        mtx.unlock();
        return false;
    }

    for ( size_t i = 0; i < count; i++ )
    {
        if ( queue[ i ].token == token )
        {
            mtx.unlock();
            return true;
        }
    }

    mtx.unlock();

    return false;
}

bool lzcomp::lz_queue::push(uint8_t *buf, const size_t &length, int32_t &token)
{
    token = -1;

    if ( buf == nullptr || length == 0 )
    {
        log(lzcomplog, "Could not push item due to invalid arguments");
        return false;
    }

    mtx.lock();

    if ( count >= size )
    {
        size += 10;

        if ( queue == nullptr )
        {
            if ( (queue = (lzQeueData_t*)malloc(size * sizeof(lzQeueData_t))) == nullptr )
            {
                mtx.unlock();
                log(lzcomplog, "Could not allocate lz queue");
                return false;
            }
        }
        else
        {
            lzQeueData_t *tmp = (lzQeueData_t*)realloc((void*)queue, size * sizeof(lzQeueData_t));

            if ( tmp == nullptr )
            {
                mtx.unlock();
                log(lzcomplog, "Could not re-allocate lz queue");
                return false;
            }

            queue = tmp;
        }

        // Make sure new slots are cleared
        memset((void*)&queue[ size - 10 ], 0, 10 * sizeof(lzQeueData_t));

        for ( size_t i = (size - 10); i < size; i++)
        {
            queue[ i ].token = -1;
        }
    }

    token = (int32_t)(tokenCntr++ & 0x7fffffff);

    queue[ count ].token = token;
    queue[ count ].buffer.data = buf;
    queue[ count ].buffer.length = length;
    memset((void*)&queue[ count ].lzData, 0, sizeof(lzData_t));
    count++;

    mtx.unlock();

    return true;
}

// Pop LZ data
bool lzcomp::lz_queue::popLz(volatile bool &threadStatus, lzData_t &data, const int32_t &token)
{
    if ( token < 0 )
    {
        log(lzcomplog, "Tried to pop invalid token");
        return false;
    }

    while ( 1 )
    {
        bool tokenFound = false;
        bool dataDone = false;
        size_t atIdx = 0;

        mtx.lock();

        if ( queue != nullptr )
        {
            for ( size_t i = 0; i < count; i++ )
            {
                if ( queue[ i ].token == token )
                {
                    if ( queue[ i ].lzData.data != nullptr )
                        dataDone = true;

                    tokenFound = true;
                    atIdx = i;
                    break;
                }
            }

            if ( tokenFound == false )
            {
                mtx.unlock();
                log(lzcomplog, "Requested token not found in queue");
                return false;
            }

            if ( dataDone )
            {
                // Copy item in question
                memcpy(&data, (void*)&queue[ atIdx ].lzData, sizeof(lzData_t));

                // Replace this item with the last one (unless this IS the last one)
                if ( (count - 1) != atIdx )
                {
                    memcpy((void*)&queue[ atIdx ],
                           (void*)&queue[ count - 1 ], sizeof(lzQeueData_t));
                }

                // Clear the last item and decrease count
                memset((void*)&queue[ count - 1 ], 0, sizeof(lzQeueData_t));
                queue[ count - 1 ].token = -1;

                count--;

                mtx.unlock();
                return true;
            }
        }
        else if ( threadStatus == false )
        {
            mtx.unlock();
            log(lzcomplog, "There is no queue");
            return false;
        }

        // Release mutex for a while (the compressor thread can't otherwise update status)
        mtx.unlock();

        timer::sleepMilli( 50 );
    }
}

// Get LZ data
bool lzcomp::lz_queue::getLz(volatile bool &threadStatus, lzData_t &data, const int32_t &token)
{
    if ( token < 0 )
    {
        log(lzcomplog, "Tried to get invalid token");
        return false;
    }

    while ( 1 )
    {
        bool tokenFound = false;
        bool dataDone = false;
        size_t atIdx = 0;

        mtx.lock();

        if ( queue != nullptr )
        {
            for ( size_t i = 0; i < count; i++ )
            {
                if ( queue[ i ].token == token )
                {
                    if ( queue[ i ].lzData.data != nullptr )
                        dataDone = true;

                    tokenFound = true;
                    atIdx = i;
                    break;
                }
            }

            if ( tokenFound == false )
            {
                mtx.unlock();
                log(lzcomplog, "Requested token not found in queue");
                return false;
            }

            if ( dataDone )
            {
                // Copy item in question
                memcpy(&data, (void*)&queue[ atIdx ].lzData, sizeof(lzData_t));

                mtx.unlock();
                return true;
            }
        }
        else if ( threadStatus == false )
        {
            mtx.unlock();
            log(lzcomplog, "There is no queue");
            return false;
        }

        // Release mutex for a while (the compressor thread can't otherwise update status)
        mtx.unlock();

        timer::sleepMilli( 50 );
    }
}

bool lzcomp::lz_queue::waiting(lzQeueData_t &queueItem)
{
    mtx.lock();

    if ( queue != nullptr )
    {
        for ( size_t i = 0; i < count; i++ )
        {
            if ( queue[i].lzData.data == nullptr )
            {
                // Copy item in question
                memcpy(&queueItem, (void*)&queue[i], sizeof(lzQeueData_t));

                mtx.unlock();
                return true;
            }
        }
    }

    mtx.unlock();
    return false;
}

bool lzcomp::lz_queue::update(const lzQeueData_t &queueItem, uint8_t *buf, const size_t &length)
{
    if ( buf == nullptr || length == 0 || queueItem.token < 0 )
    {
        log(lzcomplog, "Could not update item due to invalid arguments");
        return false;
    }

    mtx.lock();

    if ( queue != nullptr )
    {
        for ( size_t i = 0; i < count; i++ )
        {
            if ( queue[i].token == queueItem.token )
            {
                if ( queue[i].lzData.data != nullptr )
                {
                    delete[] queue[i].lzData.data;
                }

                queue[i].lzData.data = buf;
                queue[i].lzData.length = length;

                mtx.unlock();
                return true;
            }
        }
    }

    log(lzcomplog, "Could not find the correct token to update");

    mtx.unlock();
    return false;
}
