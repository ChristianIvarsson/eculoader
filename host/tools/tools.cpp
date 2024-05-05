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
                            uint8_t *str1,
                            uint8_t *str2,
                            uint32_t minlen,
                            uint32_t maxlen)
{
	for ( ; (minlen < maxlen) && (str1[minlen] == str2[minlen]); minlen++);
	return minlen;
}

uint32_t LZ_Compress(uint8_t *in, uint8_t *out, int32_t insize)
{
	// Do we have anything to compress?
	if ( insize == 0 )
    {
        log(lzcomplog, "Can't compress 0 bytes");
		return 0;
    }

	// Start of compression
	uint8_t *flags = out;
	*flags = 0;
	uint8_t mask = 0x80;

	int32_t  bytesleft = insize;
	int32_t  inpos = 0;
	uint32_t outpos = 1;

	// Main compression loop
	do {
		// Determine most distant position
		uint32_t maxoffset = (inpos > LZ_MAX_OFFSET) ? LZ_MAX_OFFSET : inpos;

		// Get pointer to current position
		uint8_t *ptr1 = &in[ inpos ];

		// Search history window for maximum length string match
		uint32_t bestlength = 2;
		uint32_t bestoffset = 0;

		for ( uint32_t offset = 3; offset <= maxoffset; offset++ )
		{
			// Get pointer to candidate string
			uint8_t *ptr2 = &ptr1[ -offset ];

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

	return outpos;
}

lzcomp::lzcomp()
{
    log(lzcomplog, "lzcomp()");

    index = count = 0;
    threadAct = false;
    queue = nullptr;
    queueSize = 0;
}

lzcomp::~lzcomp()
{
	flush();

	mtx.lock();

	if ( queue != nullptr )
	{
		free( (void*)queue );
		queue = nullptr;
	}

	mtx.unlock();
}

void lzcomp::flush()
{
	mtx.lock();

	// -:stop worker:-
	// ..
	// ..

	if ( queue != nullptr )
	{
		for ( size_t i = 0; i < count; i++ )
		{
			if ( queue[ ((index - 1) - i) % queueSize ].dst != nullptr )
			{
				delete[] queue[ ((index - 1) - i) % queueSize ].dst;
			}
		}

		for ( size_t i = 0; i < queueSize; i++ )
		{
			queue[ i ].dst = nullptr;
			queue[ i ].src = nullptr;
		}

		count = 0;
	}

	mtx.unlock();
}
