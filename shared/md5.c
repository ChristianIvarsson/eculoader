//////////////////////////////
// This clusterfuck is used to hash md5
#include "md5.h"

//
#pragma GCC push_options
#pragma GCC optimize ("Og")

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Constants, target specific etc

// mpc5566, what can I say?
// It'll crap out if you read an address that has not been correctly written
#if defined(safereads) || defined(mpc5566)
extern uint8_t __attribute__ ((noinline)) ReadData(uint32_t ptr);
#else
#define ReadData(d) \
    *(volatile uint8_t*)(d)
#endif

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Code

// Perform a full byteswap of a Dword
#define bs32(n) \
    ((n) << 24 | (( (n) >> 8) & 0xFF) << 16 | (((n) >> 16) & 0xFF) << 8 | (((n) >> 24) & 0xFF))

#define  FG(B, C, D)  ( (D) ^ ( (B) & ( (C) ^ (D) ) ) )
#define   H(B, C, D)  ( ( (B) ^ (C) ) ^ (D) )
#define   I(B, C, D)    ( (C) ^ ( (B) |~(D) ) )

#define trnsf(f, a, b, c, d, n, dat, key, shft) \
    (a) += f( (b), (c), (d) ) + (dat) + (key);   \
    (a)  = ((a) << (shft) | ( (a) & 0xFFFFFFFF ) >> (32- (shft)) ) + (n);

#ifndef POWERPC

typedef struct __attribute((packed)){
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} md5k_t;

static void mdInitKeys(md5k_t *m)
{
    m->a = 0x67452301;
    m->b = 0xefcdab89;
    m->c = 0x98badcfe;
    m->d = 0x10325476;
}

static void mdHash(md5k_t *m, uint32_t Start, uint32_t Length)
{
    uint32_t A, B, C, D;
    uint8_t buf[64] __attribute__((aligned(4)));
    uint32_t *pntr = (uint32_t *)&buf[0];

    do
    {
        A = m->a;
        B = m->b;
        C = m->c;
        D = m->d;

        for (uint32_t i = 0; i < 64; i++)
            buf[ i ^ 3 ] = ReadData( Start++ );

        trnsf(FG, A, B, C, D, B, pntr[ 0], 0xd76aa478,  7)
        trnsf(FG, D, A, B, C, A, pntr[ 1], 0xe8c7b756, 12)
        trnsf(FG, C, D, A, B, D, pntr[ 2], 0x242070db, 17)
        trnsf(FG, B, C, D, A, C, pntr[ 3], 0xc1bdceee, 22)
        trnsf(FG, A, B, C, D, B, pntr[ 4], 0xf57c0faf,  7)
        trnsf(FG, D, A, B, C, A, pntr[ 5], 0x4787c62a, 12)
        trnsf(FG, C, D, A, B, D, pntr[ 6], 0xa8304613, 17)
        trnsf(FG, B, C, D, A, C, pntr[ 7], 0xfd469501, 22)
        trnsf(FG, A, B, C, D, B, pntr[ 8], 0x698098d8,  7)
        trnsf(FG, D, A, B, C, A, pntr[ 9], 0x8b44f7af, 12)
        trnsf(FG, C, D, A, B, D, pntr[10], 0xffff5bb1, 17)
        trnsf(FG, B, C, D, A, C, pntr[11], 0x895cd7be, 22)
        trnsf(FG, A, B, C, D, B, pntr[12], 0x6b901122,  7)
        trnsf(FG, D, A, B, C, A, pntr[13], 0xfd987193, 12)
        trnsf(FG, C, D, A, B, D, pntr[14], 0xa679438e, 17)
        trnsf(FG, B, C, D, A, C, pntr[15], 0x49b40821, 22)

        trnsf(FG, A, D, B, C, B, pntr[ 1], 0xf61e2562,  5)
        trnsf(FG, D, C, A, B, A, pntr[ 6], 0xc040b340,  9)
        trnsf(FG, C, B, D, A, D, pntr[11], 0x265e5a51, 14)
        trnsf(FG, B, A, C, D, C, pntr[ 0], 0xe9b6c7aa, 20)
        trnsf(FG, A, D, B, C, B, pntr[ 5], 0xd62f105d,  5)
        trnsf(FG, D, C, A, B, A, pntr[10], 0x02441453,  9)
        trnsf(FG, C, B, D, A, D, pntr[15], 0xd8a1e681, 14)
        trnsf(FG, B, A, C, D, C, pntr[ 4], 0xe7d3fbc8, 20)
        trnsf(FG, A, D, B, C, B, pntr[ 9], 0x21e1cde6,  5)
        trnsf(FG, D, C, A, B, A, pntr[14], 0xc33707d6,  9)
        trnsf(FG, C, B, D, A, D, pntr[ 3], 0xf4d50d87, 14)
        trnsf(FG, B, A, C, D, C, pntr[ 8], 0x455a14ed, 20)
        trnsf(FG, A, D, B, C, B, pntr[13], 0xa9e3e905,  5)
        trnsf(FG, D, C, A, B, A, pntr[ 2], 0xfcefa3f8,  9)
        trnsf(FG, C, B, D, A, D, pntr[ 7], 0x676f02d9, 14)
        trnsf(FG, B, A, C, D, C, pntr[12], 0x8d2a4c8a, 20)

        trnsf(H , A, B, C, D, B, pntr[ 5], 0xfffa3942,  4)
        trnsf(H , D, A, B, C, A, pntr[ 8], 0x8771f681, 11)
        trnsf(H , C, D, A, B, D, pntr[11], 0x6d9d6122, 16)
        trnsf(H , B, C, D, A, C, pntr[14], 0xfde5380c, 23)
        trnsf(H , A, B, C, D, B, pntr[ 1], 0xa4beea44,  4)
        trnsf(H , D, A, B, C, A, pntr[ 4], 0x4bdecfa9, 11)
        trnsf(H , C, D, A, B, D, pntr[ 7], 0xf6bb4b60, 16)
        trnsf(H , B, C, D, A, C, pntr[10], 0xbebfbc70, 23)
        trnsf(H , A, B, C, D, B, pntr[13], 0x289b7ec6,  4)
        trnsf(H , D, A, B, C, A, pntr[ 0], 0xeaa127fa, 11)
        trnsf(H , C, D, A, B, D, pntr[ 3], 0xd4ef3085, 16)
        trnsf(H , B, C, D, A, C, pntr[ 6], 0x04881d05, 23)
        trnsf(H , A, B, C, D, B, pntr[ 9], 0xd9d4d039,  4)
        trnsf(H , D, A, B, C, A, pntr[12], 0xe6db99e5, 11)
        trnsf(H , C, D, A, B, D, pntr[15], 0x1fa27cf8, 16)
        trnsf(H , B, C, D, A, C, pntr[ 2], 0xc4ac5665, 23)

        trnsf(I , A, B, C, D, B, pntr[ 0], 0xf4292244,  6)
        trnsf(I , D, A, B, C, A, pntr[ 7], 0x432aff97, 10)
        trnsf(I , C, D, A, B, D, pntr[14], 0xab9423a7, 15)
        trnsf(I , B, C, D, A, C, pntr[ 5], 0xfc93a039, 21)
        trnsf(I , A, B, C, D, B, pntr[12], 0x655b59c3,  6)
        trnsf(I , D, A, B, C, A, pntr[ 3], 0x8f0ccc92, 10)
        trnsf(I , C, D, A, B, D, pntr[10], 0xffeff47d, 15)
        trnsf(I , B, C, D, A, C, pntr[ 1], 0x85845dd1, 21)
        trnsf(I , A, B, C, D, B, pntr[ 8], 0x6fa87e4f,  6)
        trnsf(I , D, A, B, C, A, pntr[15], 0xfe2ce6e0, 10)
        trnsf(I , C, D, A, B, D, pntr[ 6], 0xa3014314, 15)
        trnsf(I , B, C, D, A, C, pntr[13], 0x4e0811a1, 21)
        trnsf(I , A, B, C, D, B, pntr[ 4], 0xf7537e82,  6)
        trnsf(I , D, A, B, C, A, pntr[11], 0xbd3af235, 10)
        trnsf(I , C, D, A, B, D, pntr[ 2], 0x2ad7d2bb, 15)
        trnsf(I , B, C, D, A, C, pntr[ 9], 0xeb86d391, 21)

        m->a += A;
        m->b += B;
        m->c += C;
        m->d += D;

    } while (Length -= 64);
}

void *hashMD5(const uint32_t addr, const uint32_t len)
{
    uint8_t   buf[64] __attribute__((aligned(4)));
    uint32_t *stfu  = (uint32_t*)&buf[56]; // Since, f*ck that warning
    uint8_t   left  = len & 0x3F;
    uint32_t  Start = addr;

    static md5k_t md5prm;
    mdInitKeys(&md5prm);

    if (len > 63)
        mdHash(&md5prm, Start, (len & ~63));

    // Fill buffer with left over data (If applicable)
    Start = Start + (len - left);
    for (uint32_t i = 0; i < left; i++)
        buf[i] = ReadData(Start++);

    // Append 1 bit
    buf[left] = 0x80;

	// We must have enough room for the len-bytes
	if (++left > 56)
	{
	    for ( ; left < 64; left++)
	        buf[left] = 0;
	    mdHash(&md5prm, (uint32_t)&buf[0],  64);
	    left = 0; 
	}

    for ( ; left < 64; left++)
        buf[left] = 0;

	*stfu = bs32(len << 3);
    mdHash(&md5prm, (uint32_t)&buf[0], 64);

    md5prm.a = bs32(md5prm.a);
    md5prm.b = bs32(md5prm.b);
    md5prm.c = bs32(md5prm.c);
    md5prm.d = bs32(md5prm.d);

    return &md5prm;
}

#else

extern void mdHash(void *m);

static void mdInitKeys(uint32_t *m)
{
    m[2] = 0x67452301; // A
    m[3] = 0xefcdab89; // B
    m[4] = 0x98badcfe; // C
    m[5] = 0x10325476; // D
}

void *hashMD5(const uint32_t addr, const uint32_t len)
{
    uint8_t   buf[64] __attribute__((aligned(4)));
    uint32_t *stfu  = (uint32_t*)&buf[56]; // Since, f*ck that warning
    uint8_t   left  = len & 0x3F;
    uint32_t  Start = addr;

    static uint32_t mdarray[6];
    mdInitKeys(mdarray);

    if (len > 63) {
        mdarray[0] = addr;
        mdarray[1] = (len & ~63);
        mdHash(mdarray);
    }

    // Fill buffer with left over data (If applicable)
    Start = Start + (len - left);
    for (uint32_t i = 0; i < left; i++)
        buf[i] = ReadData(Start++);

    // Append 1 bit
    buf[left] = 0x80;

	// We must have enough room for the len-bytes
	if (++left > 56)
	{
	    for ( ; left < 64; left++)
	        buf[left] = 0;

        mdarray[0] = (uint32_t) &buf[0];
        mdarray[1] = 64;
        mdHash(mdarray);
	    left = 0; 
	}

    for ( ; left < 64; left++)
    {
        buf[left] = 0;
    }

	*stfu = bs32(len << 3);

    mdarray[0] = (uint32_t) &buf[0];
    mdarray[1] = 64;
    mdHash(mdarray);

    mdarray[2] = bs32(mdarray[2]);
    mdarray[3] = bs32(mdarray[3]);
    mdarray[4] = bs32(mdarray[4]);
    mdarray[5] = bs32(mdarray[5]);

    return &mdarray[2];
}

#endif



#pragma GCC pop_options
